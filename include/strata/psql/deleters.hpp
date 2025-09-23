#pragma once
#include <string>
#include <fstream>
#include "../db_config.hpp"

#ifdef PSQL
namespace psql {

void drop_table(const std::string& model_name, std::ofstream& Migrations);

void drop_column(const std::string& model_name, const std::string& column_name, std::ofstream& Migrations);

void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations);

}

namespace db_adapter = psql;
#endif
