#pragma once
#include "../db_config.hpp"
#include <string>
#include <fstream>

#ifdef PSQL
namespace psql {
void alter_rename_table(const std::string& old_model_name, const std::string& new_model_name, std::ofstream& Migrations);

void alter_add_column(const std::string& model_name, const std::string& column_name,
                      const std::string& column_sql_attributes, std::ofstream& Migrations);

void alter_rename_column(const std::string& model_name, const std::string& old_column_name,
                         const std::string& new_column_name, std::ofstream& Migrations);

void alter_column_type(const std::string& model_name, const std::string& column_name,
                       const std::string& sql_segment, std::ofstream& Migrations);

void alter_column_defaultval(const std::string& model_name, const std::string& column_name,
                             const bool set_default, const std::string& defaultval, std::ofstream& Migrations);

void alter_column_nullable(const std::string& model_name, const std::string& column_name, const bool nullable, std::ofstream& Migrations);

}
namespace db_adapter = psql;
#endif
