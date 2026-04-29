#pragma once
#include <strata/db_config.hpp>

#ifdef PSQL
#include <strata/psql/connectors.hpp>
#include <pqxx/row>

namespace psql {

template<typename Model_T>
pqxx::connection prepare_insert()
{
  Model_T obj {};
  pqxx::placeholders row_vals {};
  pqxx::connection cxn = connect();
  std::string insert_statement = "insert into "+ obj.table_name + " (" + obj.col_str +") values(";

  for(int i=0; i<obj.col_map_size; ++i){
    insert_statement += row_vals.get() + ",";
    row_vals.next();
  }
  insert_statement.append("\b);");
  cxn.prepare("insert_stmt", insert_statement);

  return cxn;
}

inline void exec_insert(pqxx::connection& cxn, pqxx::params& row)
{
  try{
    pqxx::work txn(cxn);
    pqxx::result result = txn.exec(pqxx::prepped{"insert_stmt"}, row).no_rows();
    txn.commit();
  }catch(const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'exec_insert()'] => {}.", e.what()));
  }
}

}
namespace db = psql;
#endif
