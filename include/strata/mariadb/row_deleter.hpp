#pragma once
#include "../db_config.hpp"

#ifdef MARIADB
#include "executor.hpp"

namespace mariadb {

template <typename Model_T>
void delete_row(std::string logical_op, Utils::filters& filters)
{
  Model_T obj {};
  std::string sql_str {"delete from " + obj.table_name + " where " + Utils::build_filter_args(logical_op, filters) + ";"};
  opt_result_t result = execute_sql(sql_str, false);
}

} //INFO: namespace mariadb
namespace db = mariadb;
#endif
