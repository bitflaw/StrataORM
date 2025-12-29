#include "../../include/strata/mariadb/alterers.hpp"
#include <iostream>

namespace mariadb {

void alter_rename_table(const std::string& old_model_name, const std::string& new_model_name, std::ofstream& Migrations){
  Migrations<< "ALTER TABLE " + old_model_name + " RENAME TO " + new_model_name + ";\n";
}

void alter_add_column(const std::string& model_name, const std::string& column_name,
                      const std::string& column_sql_attributes, std::ofstream& Migrations){
  Migrations<< "ALTER TABLE " + model_name + " ADD COLUMN IF NOT EXISTS " + column_name + " " + column_sql_attributes + ";\n";
}

void alter_rename_column(const std::string& model_name, const std::string& old_column_name,
                         const std::string& new_column_name, std::ofstream& Migrations){
  Migrations << "ALTER TABLE " + model_name + " RENAME COLUMN IF EXISTS " + old_column_name + " TO " + new_column_name + ";\n";
}

void alter_column_type(const std::string& model_name, const std::string& column_name,
                       const std::string& sql_segment, std::ofstream& Migrations){
  Migrations<< "ALTER TABLE " + model_name + " MODIFY " + column_name + " " + sql_segment + ";\n";
}

void alter_column_defaultval(const std::string& model_name, const std::string& column_name,
                             const bool set_default, const std::string& defaultval, std::ofstream& Migrations){
  if(set_default){
    Migrations << "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET DEFAULT " + defaultval + ";\n";
  }else{
    Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP DEFAULT;\n";
  }
}

//WARNING: Can't implement yet because mariadb's version of altering column nullability requires full column redefinition...
void alter_column_nullable(const std::string& model_name, const std::string& column_name, const bool nullable, std::ofstream& Migrations){
  if(nullable){
    Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP NOT NULL;\n";
  }else{
    std::string default_value;
    std::cout<<"Provide a default value for the column '" + column_name +"' to be set to non-nullable: " << std::endl;
    std::cin>> default_value;
    Migrations<< std::boolalpha
      << "UPDATE " + model_name + " SET " + column_name + " = '" + default_value + "' WHERE " + column_name
      << " IS NULL;\n ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET NOT NULL;\n";
  }
}

} // INFO: namespace mariadb
