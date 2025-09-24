#pragma once
#include "../db_config.hpp"
#include "utils.hpp"
#include <format>

#ifdef PSQL
#include <pqxx/row>
#include <pqxx/transaction>
namespace psql {

inline pqxx::connection connect(){
  Utils::db_params params = Utils::parse_dbenvars();
  try{
    pqxx::connection cxn("dbname=" + params.db_name+
                         " user=" + params.user +
                         " password=" + params.passwd +
                         " host=" + params.host +
                         " port=" + std::to_string(params.port)
                         );
    return cxn;
  }catch (const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'connect()'] => {}", e.what()));
  }
}

}
namespace db_adapter = psql;
#endif
