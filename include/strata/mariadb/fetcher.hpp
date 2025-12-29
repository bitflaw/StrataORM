#pragma once
#include "../db_config.hpp"

#ifdef MARIADB

#include "connectors.hpp"
#include <mdbcxx/transaction.hpp>

namespace mariadb
{

template<typename Model_T>
void dbfetch(Model_T& obj, std::string& sql_string, bool getfn_called = false)
{
  try
  {
    mcxx::Connection cxn = connect();
    mcxx::Transaction txn {cxn};

    if(getfn_called)
    {
      mcxx::Row row = txn.exec1(sql_string);
      obj.records.push_back(row);
      return;
    }
    std::optional<mcxx::Result> result = txn.exec(sql_string);
    if (!result.has_value()) return;
    for (mcxx::Row& row : result.value()) obj.records.push_back(row);

  } catch (const std::exception& e)
  {
    throw std::runtime_error(e.what());
  }
}

}//namespace mariadb
namespace db = mariadb;
#endif
