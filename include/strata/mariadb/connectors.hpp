#pragma once
#include <strata/db_config.hpp>

#ifdef MARIADB

#include <exception>
#include <mdbcxx/connection.hpp>
#include <strata/utils.hpp>

namespace mariadb
{
inline mcxx::Connection connect ()
{
  try
  {
    Utils::db_params params = Utils::parse_dbenvars();
    mcxx::Properties props {};
    props.db_name = params.db_name;
    props.host = params.host;
    props.passwd = params.passwd;
    props.port = params.port;
    props.user = params.user;
    mcxx::Connection cxn {props};
    return cxn;
  } catch (std::exception& e)
  {
    throw std::runtime_error (e.what());
  } catch (...) {
    throw std::runtime_error("Failed to connect to the database!");
  }
}

}
namespace db = mariadb;
#endif
