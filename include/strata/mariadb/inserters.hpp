#pragma once
#include "../db_config.hpp"
#include <mdbcxx/connection.hpp>
#include <mdbcxx/prepped.hpp>

#ifdef MARIADB

#include "connectors.hpp"
#include <mdbcxx/transaction.hpp>

namespace mariadb
{
template<typename Model_T>
mcxx::prepped_stmt prepare_insert()
{
  Model_T obj {};
  mcxx::Connection cxn = connect();
  std::string insert_statement = "insert into "+ obj.table_name + " (" + obj.col_str +") values(";

  for(int i=0; i<obj.col_map_size; i++) insert_statement +=  "?,";
  insert_statement.append("\b);");
  cxn.prepare("insert_stmt", insert_statement);
  return cxn.prepped("insert_stmt");
}

inline void exec_insert(mcxx::prepped_stmt& prepped_stmt, mcxx::params& prepped_params)
{
  try
  {
    mcxx::Connection cxn {connect()};
    mcxx::Transaction txn {cxn};
    txn.exec0(prepped_stmt, prepped_params);
  }catch(const std::exception& e)
  {
    throw std::runtime_error(e.what());
  }
}

} //INFO: namespace mariadb
namespace db = mariadb;
#endif
