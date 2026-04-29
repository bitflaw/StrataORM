#pragma once
#include <strata/db_config.hpp>

#ifdef PSQL

#include <strata/utils.hpp>
#include <format>
#include <pqxx/connection>

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
namespace db = psql;
#endif
