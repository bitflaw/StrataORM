#include "../db_config.hpp"

#ifdef MARIADB
#include <string>
#include <fstream>

namespace mariadb {

void drop_table(const std::string& model_name, std::ofstream& Migrations);

void drop_column(const std::string& model_name, const std::string& column_name,  std::ofstream& Migrations);

void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations);

}// INFO: namespace mariadb

namespace db = mariadb;
#endif
