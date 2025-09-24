#pragma once
#include "../db_config.hpp"
#include "connectors.hpp"
#include <fstream>

#ifdef PSQL
#include <pqxx/transaction>
#include <pqxx/result>

namespace psql {

using opt_result_t = std::optional<pqxx::result>;
inline opt_result_t execute_sql(std::string& sql_file_or_str, bool is_file_name = true){
  std::ostringstream raw_sql {};

  if(is_file_name){
    std::ifstream sql_file(sql_file_or_str);
    if(!sql_file.is_open()) throw std::runtime_error("[ERROR: in 'execute_sql()'] => Couldn't open the sql file to which the path is provided.");
    raw_sql << sql_file.rdbuf();
  }else{
    raw_sql << sql_file_or_str;
  }

  Utils::db_params params = Utils::parse_dbenvars();

  try{
    pqxx::connection cxn = connect();
    pqxx::work txn(cxn);
    pqxx::result results = txn.exec(raw_sql.str());
    txn.commit();
    if(!results.empty()) return results;
    return std::nullopt;
  }catch (const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'execute_sql()'] => {}", e.what()));
  }
}

}
namespace db_adapter = psql;
#endif
