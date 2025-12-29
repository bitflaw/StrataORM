#pragma once
#include "../db_config.hpp"

#ifdef MARIADB
#include "../custom_array.hpp"
#include "connectors.hpp"
#include <format>
#include <mdbcxx/row.hpp>
#include <mdbcxx/transaction.hpp>

namespace mariadb::query {

template <typename Model_T>
void fetch_all(Model_T& obj, std::string columns){
  std::string sql_string {"select " + columns + " from " + obj.table_name + ";"};
  dbfetch(obj, sql_string);
}

template <typename Model_T, typename... Args>
void get(Model_T& obj, Args... args){
  static_assert(sizeof...(args) > 0 || sizeof...(args)%2 == 0,
                "[ERROR:'db_adapter::query::get()'] Args are provided in key-value pairs."
  );

  std::string sql_kwargs {};
  constexpr int N = sizeof...(args);
  Utils::CustomArray<std::pair<std::string, std::string>, N/2> kwargs {};
  Utils::CustomArray<std::string, N> parsed_args {Utils::to_str(args)...};

  for(int i = 0; i < N; i+=2){
    sql_kwargs += parsed_args[i] + "=" + parsed_args[i+1] + " and ";
    kwargs.push_back(std::make_pair(parsed_args[i], parsed_args[i+1]));
  }

  sql_kwargs.replace(sql_kwargs.size()-5, 5, ";");

  if(obj.records.empty()){
    std::string sql_str = "select * from " + obj.table_name + " where " + sql_kwargs;
    dbfetch(obj, sql_str, true);
  }else{
    std::vector<mcxx::Row> filtered_rows {};
    for(const mcxx::Row& row : obj.records)
    {
      bool accept_row = true;
      for(const auto& kwarg : kwargs){
        if(row[kwarg.first].template as<std::string>() != kwarg.second){
          accept_row = false;
          break;
        }
      }
      if(accept_row){
        filtered_rows.push_back(row);
        continue;
      }
      if(filtered_rows.size() == 1) break;
    }
    obj.records = filtered_rows;
  }
}

inline bool matches_conditions(mcxx::Field& field, OP op, Utils::Value_T v)
{
  bool accept = false;
  std::any value = Utils::filter_val(v);
  int int_cast = 0;
  double double_cast = 0;
  std::string str_cast {};

  if(value.type() == typeid(int)) int_cast = std::any_cast<int>(value);
  else if(value.type() == typeid(double)) double_cast = std::any_cast<double>(value);
  else if(value.type() == typeid(std::string)) str_cast = std::any_cast<std::string>(value);
  else throw std::invalid_argument("[ERROR: 'filter().match_conditions()'] => Unsupported type passed to filters");

  try{
    switch (op) {
      case EQ:
        if(int_cast) accept = (field.as<int>() == int_cast);
        else if(double_cast) accept = (field.as<double>() == double_cast);
        else if(!str_cast.empty()) accept = (field.as<std::string>().find(str_cast) != std::string::npos);
        else throw std::runtime_error("Unsupported type for OP::EQ");
        break;
      case GT:
        if(int_cast) accept = (field.as<int>() > int_cast);
        else if(double_cast) accept = (field.as<double>() > double_cast);
        else throw std::runtime_error("Unsupported type for OP::GT)");
        break;
      case GTE:
        if(int_cast) accept = (field.as<int>() >= int_cast);
        else if(double_cast) accept = (field.as<double>() >= double_cast);
        else throw std::runtime_error("Unsupported type for OP::GTE)");
        break;
      case LT:
        if(int_cast) accept = (field.as<int>() < int_cast);
        else if(double_cast) accept = (field.as<double>() < double_cast);
        else throw std::runtime_error("Unsupported type for OPERAND 'OP::LT'");
        break;
      case LTE:
        if(int_cast) accept = (field.as<int>() <= int_cast);
        else if(double_cast) accept = (field.as<double>() <= double_cast);
        else throw std::runtime_error("Unsupported type for OPERAND 'OP::LTE'");
        break;
      case LIKE:
      case ILIKE:
        throw std::runtime_error("LIKE/ILIKE not implemented yet for filtering from obj.records.");
        break;
      case STARTSWITH:
      case CONTAINS:
        if(!str_cast.empty()) accept = (field.as<std::string>().find(str_cast) != std::string::npos);
        else throw std::runtime_error("Unsupported type for OPERAND(OP::STARTSWITH || OP::CONTAINS)");
        break;
      case ENDSWITH:
        if(!str_cast.empty()){
          std::string field_str = field.as<std::string>();
          accept = (field.as<std::string>().find(str_cast, field_str.size() - str_cast.size()) != std::string::npos);
        } else throw std::runtime_error("Unsupported type for OP::ENDSWITH");
        break;
      default:
        throw std::runtime_error("Unknown operator!");
    }
  }catch(const std::exception& e){
    throw std::runtime_error(e.what());
  }
  return accept;
}

template <typename Model_T>
void filter(Model_T& obj, std::string logical_op, Utils::filters& filters)
{
  if(obj.records.empty())
  {
    std::string sql_str = "select * from " + obj.table_name + " where " + build_filter_args(logical_op, filters) + ";";
    dbfetch(obj, sql_str);
  }else{
    std::vector<mcxx::Row> filtered_rows {};
    if (logical_op == "and")
    {
      for(mcxx::Row& row : obj.records)
      {
        bool accept_row = true;
        for(Utils::Condition& filter: filters)
        {
          if(!matches_conditions(row[filter.column], filter.op, filter.value))
          {
            accept_row = false;
            break;
          }
        }
        if(accept_row)
        {
          filtered_rows.push_back(row);
          continue;
        }
      }
    }else if(logical_op == "or"){
      for(mcxx::Row& row : obj.records)
      {
        bool accept_row = false;
        for(Utils::Condition& filter: filters)
        {
          if(matches_conditions(row[filter.column], filter.op, filter.value)){
            accept_row = true;
            break;
          }
        }
        if(accept_row)
        {
          filtered_rows.push_back(row);
          continue;
        }
      }
    }else {
      throw std::runtime_error("Unknown logical operator for filter fn");
    }

    if(filtered_rows.size() <= obj.records.size()) obj.records = filtered_rows;
    else throw std::runtime_error("Filtered rows are more than the actual initial rows!");
  }
}

class JoinBuilder
{
  std::string query_str {}, table_name {};
  bool join_pending = true;
public:
  template<typename T>
  JoinBuilder(T& model): table_name(model.table_name) {}

  template <typename... Args>
  requires all_same_as_T<std::string, Args...> || all_convertible_to_T<std::string, Args...>
  JoinBuilder& select(Args&&... columns){
    query_str = "select " + ((to_str(columns) + ",") + ...);
    query_str.pop_back();
    query_str += " from " + table_name;
    return *this;
  }

  JoinBuilder& inner_join(std::string join_table){
    query_str += " inner join " + join_table;
    if(!join_pending)
      throw std::runtime_error("You have not implemented .on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& outer_join(std::string join_table){
    query_str += " outer join " + join_table;
    if(!join_pending)
      throw std::runtime_error("You have not implemented .on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& full_join(std::string join_table){
    query_str += " full join " + join_table;
    if(!join_pending)
      throw std::runtime_error("You have not implemented .on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& left_join(std::string join_table){
    query_str += " left join " + join_table;
    if(!join_pending)
      throw std::runtime_error("You have not implemented .on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& right_join(std::string join_table){
    query_str += " right join " + join_table;
    if(!join_pending)
      throw std::runtime_error("You have not implemented .on() yet for the previous join!");
    join_pending = false;
    return *this;
  }

  template<typename... Args>
  requires all_same_as_T<std::string, Args...> || all_convertible_to_T<std::string, Args...>
  JoinBuilder& on(std::string logical_op, Args&&... conditions){
    if(join_pending) throw std::runtime_error("Join pending");
    if(logical_op != "and" && logical_op != "or")
      throw std::runtime_error(std::format("Unknown logical operator: {}", logical_op));
    query_str += " on " + ((to_str(conditions) + " " + logical_op + " ") + ...);
    query_str.resize(query_str.size() - (logical_op.size() + 2));
    join_pending = true;
    return *this;
  }

  mcxx::Result execute()
  {
    try{
      mcxx::Connection cxn = connect();
      mcxx::Transaction txn {cxn};
      std::optional<mcxx::Result> join_results = txn.exec(query_str + ";");
      txn.commit();
      return join_results.value_or({});
    }catch(const std::exception& e){
      throw std::runtime_error(e.what());
    }
  }

  std::string str(){ return query_str + ";"; }
};

}//INFO: namespace mariadb::query
namespace db = mariadb;
#endif
