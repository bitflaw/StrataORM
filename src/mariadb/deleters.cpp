#include <strata/mariadb/deleters.hpp>

namespace mariadb {

void drop_table(const std::string& model_name, std::ofstream& Migrations)
{
  Migrations << "DROP TABLE IF EXISTS " + model_name + ";\n";
}

void drop_column(const std::string& model_name, const std::string& column_name,  std::ofstream& Migrations)
{
  Migrations << "ALTER TABLE " + model_name + " DROP COLUMN IF EXISTS " + column_name + ";\n";
}

void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations)
{
  Migrations<< "ALTER TABLE " + model_name + " DROP CONSTRAINT IF EXISTS " + constraint_name + ";\n";
}

}// INFO: namespace mariadb
