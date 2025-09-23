#include "../../include/strata/psql/utils.hpp"
#include <exception>
#include <format>
#include <stdexcept>

namespace Utils {

std::string str_to_upper(std::string& str){
  for(char& ch: str){
    ch = std::toupper(ch);
  }
  return str;
}

void set_dbenvars(Utils::dbenvars& params){
  try{
    for(auto kvparam: params) {
      int retval = setenv(kvparam.first.data(), kvparam.second.data(), 1);
      if (retval != 0) throw std::runtime_error("Failed to set environmental parameters! Unable to continue");
    }
  }catch(std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'set_dbenvars()'] => {}", e.what()));
  }
}

db_params parse_dbenvars(){
  try {
    db_params params {
      getenv("DBNAME"),
      getenv("DBUSER"),
      getenv("DBPASS"),
      getenv("DBHOST"),
      std::stoi(getenv("DBPORT"))
    };
    return params;
  } catch (std::exception& e) {
    throw std::runtime_error(std::format("[ERROR: In 'parse_db_envars()'] => {}", e.what()));
  }
}

}
