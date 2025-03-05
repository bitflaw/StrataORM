#pragma once
#include <cstddef>
#include <string>
#include <fstream>

std::string to_upper(std::string& str);

namespace psql{

  template <typename Col_Map>
  void create_table(std::string& model_name, Col_Map& fields, std::ofstream& Migrations);

  void create_column(std::string& column_name, std::string& column_sql_attributes, std::ofstream& Migrations);

  //NOTE under consideration
  void create_constraint(std::string& model_name, std::string& constraint_type,
                         std::string& column_name, std::ofstream& Migrations);

  void alter_rename_table(std::string& old_model_name, std::string& new_model_name, std::ofstream& Migrations);

  void alter_add_column(std::string& model_name,std::string& column_name,
                        std::string& column_sql_attributes, std::ofstream& Migrations);

  void alter_rename_column(std::string& model_name, std::string& old_column_name, 
                           std::string& new_column_name, std::ofstream& Migrations);

  void alter_column_type(std::string& model_name, std::string& column_name,
                         std::string& sql_segment4_type, std::ofstream& Migrations);

  void alter_column_defaultval(std::string& model_name, std::string& column_name,
                               bool set_default, std::string& defaultval);

  void alter_column_nullable(std::string& model_name, std::string&  column_name, bool nullable, std::ofstream& Migrations);

  void alter_add_constraint(std::string& model_name, std::string& constraint_type,
                            std::string& column_name, std::ofstream& Migrations);

  void drop_table(std::string& model_name, std::ofstream& Migrations);

  void drop_column(std::string& model_name, std::string& column_name,  std::ofstream& Migrations);

  void drop_constraint(std::string& model_name, std::string& constraint_type,
                       std::string& column_name, std::ofstream& Migrations);

  template <typename IntType>
  void generate_int_sql(IntType& int_obj);

  template <typename CharType>
  void generate_char_sql(CharType& char_obj);

  template <typename DecimalType>
  void generate_decimal_sql(DecimalType& dec_obj);

  template <typename BoolType>
  void generate_bool_sql(BoolType& bool_obj);

  template <typename BinType>
  void generate_bin_sql(BinType& bin_obj);

  template <typename DTType>
  void generate_datetime_sql(DTType& dt_obj);

  template <typename FKType>
  void generate_foreignkey_sql(FKType& fk_obj);

}
