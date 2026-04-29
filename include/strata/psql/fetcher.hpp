#pragma once
#include <strata/db_config.hpp>

#ifdef PSQL
#include <strata/psql/connectors.hpp>
#include <pqxx/row>
#include <pqxx/transaction>

namespace psql {

template<typename Model_T>
void dbfetch(Model_T& obj, std::string& sql_string, bool getfn_called = false)
{
  pqxx::connection cxn= connect();
  try{
    pqxx::work txn(cxn);

    if(getfn_called){
      pqxx::result result = txn.exec(sql_string).expect_rows(1);
      obj.records.push_back(result[0]);
      return;
    }

    for(const pqxx::row& row : txn.exec(sql_string)) obj.records.push_back(row);

    txn.commit();
  }catch (const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'db_fetch()'] => {}", e.what()));
  }
  return;
}

}
namespace db = psql;
#endif
