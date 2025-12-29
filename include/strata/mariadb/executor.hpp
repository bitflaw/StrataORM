#pragma once
#include "../db_config.hpp"

#ifdef MARIADB

#include <fstream>
#include "connectors.hpp"
#include <mdbcxx/transaction.hpp>

namespace mariadb
{
using opt_result_t = std::optional<mcxx::Result>;
inline opt_result_t execute_sql(std::string& sql_file_or_str, bool is_file_name = true)
{
  std::ostringstream raw_sql {};

  if(is_file_name){
    std::ifstream sql_file(sql_file_or_str);
    if(!sql_file.is_open())
      throw std::runtime_error("Couldn't open the sql file to which the path is provided.");
    raw_sql << sql_file.rdbuf();
  }else{
    raw_sql << sql_file_or_str;
  }

  try{
    mcxx::Connection cxn = connect();
    mcxx::Transaction txn {cxn};
    return txn.exec(raw_sql.str());
  }catch (std::exception& e)
  {
    throw std::runtime_error(e.what());
  }catch (...)
  {
    throw std::runtime_error("SQL Execution failed!");
  }
}
}
namespace db = mariadb;
#endif
