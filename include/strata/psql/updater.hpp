#pragma once
#include "../db_config.hpp"
#include "../concepts.hpp"
#include "connectors.hpp"

#ifdef PSQL
#include <pqxx/row>

namespace psql {

template<typename Model_T>
struct Update {
  Model_T obj {};
  std::string query {"update "+ obj.table_name + " set "};
  pqxx::placeholders<int> row_vals {};
  int count {row_vals.count()};
  pqxx::params params {};
  bool update_col_called = false;

  template <typename... Args>
  requires (sizeof...(Args) > 0) && (all_convertible_to_T<std::string, Args...> || all_same_as_T<std::string, Args...>)
  Update& update_column(Args&&... args){
    ((query += std::string{std::forward<Args>(args)} + "=" + row_vals.get() + ",", row_vals.next()), ...);
    query.pop_back();
    count = row_vals.count()-1;
    update_col_called = true;
    return *this;
  }

  template <typename... Args>
  requires (sizeof...(Args) > 0) && (all_convertible_to_T<std::string, Args...> || all_same_as_T<std::string, Args...>)
  Update& set_to(Args... args){
    if(!update_col_called) throw std::logic_error(".column() must be called first to set the column to be updated!");
    (params.append(Utils::to_str(args)), ...);
    update_col_called = false;
    return *this;
  }

  Update& where (std::string logical_op, Utils::filters& filters){
    query.append(" where " + Utils::build_filter_args(logical_op, filters));
    return *this;
  }

  void commit(){
    query.append(";");
    if(params.size() <= 0 || params.size() != count)
      throw std::length_error("Unable to commit transaction, parameter values were empty");
    try{
      pqxx::connection cxn = connect();
      cxn.prepare("update_stmt", query);
      pqxx::work txn {cxn};
      pqxx::result res = txn.exec(pqxx::prepped{"update_stmt"}, params).no_rows();
      txn.commit();
    }catch(const std::exception& e){
      throw std::runtime_error(std::format("[ERROR: in psql::Update<T>::commit()] => {}", e.what()));
    }
  }
};

}
namespace db_adapter = psql;
#endif
