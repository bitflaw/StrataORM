#pragma once
#include "../db_config.hpp"

#ifdef MARIADB

#include "../concepts.hpp"
#include "connectors.hpp"
#include <mdbcxx/transaction.hpp>
#include <cstddef>


namespace mariadb
{

template<typename Model_T>
struct Update
{
  Model_T obj {};
  std::string query {"update "+ obj.table_name + " set "};
  std::size_t arg_count {0};
  mcxx::params param_list {};
  bool update_col_called = false;

  template <typename... Args>
  requires (sizeof...(Args) > 0) && (all_convertible_to_T<std::string, Args...> || all_same_as_T<std::string, Args...>)
  Update& update_column(Args&&... args){
    ((query += std::string{std::forward<Args>(args)} + "=?,"), ...);
    query.pop_back();
    arg_count += sizeof...(args);
    update_col_called = true;
    return *this;
  }

  template <typename... Args>
  requires (sizeof...(Args) > 0) && (all_convertible_to_T<std::string, Args...> || all_same_as_T<std::string, Args...>)
  Update& set_to(Args... args){
    if(!update_col_called) throw std::logic_error(".column() must be called first to set the column to be updated!");
    (param_list.append(Utils::to_str(args)), ...);
    update_col_called = false;
    return *this;
  }

  Update& where (std::string logical_op, Utils::filters& filters){
    query.append(" where " + Utils::build_filter_args(logical_op, filters));
    return *this;
  }

  void commit(){
    query.append(";");
    if(param_list.size() <= 0 || param_list.size() != arg_count)
      throw std::length_error(
        "Unable to commit transaction, number of parameter values doesn't match number of columns provided!"
      );
    try
    {
      mcxx::Connection cxn = connect();
      cxn.prepare("update_stmt", query);
      mcxx::prepped_stmt prepped {cxn.prepped("update_stmt")};
      mcxx::Transaction txn {cxn};
      txn.exec0(prepped, param_list);
    }catch(const std::exception& e)
    {
      throw std::runtime_error(e.what());
    }
  }
};

}
namespace db = mariadb;
#endif
