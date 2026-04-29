#pragma once
#include <strata/db_config.hpp>

#ifdef PSQL

#include <string>
#include <fstream>

namespace psql {

void drop_table(const std::string& model_name, std::ofstream& Migrations);

void drop_column(const std::string& model_name, const std::string& column_name, std::ofstream& Migrations);

void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations);

}

namespace db = psql;
#endif
