#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "./db_config.hpp"

typedef struct{
  std::string db_name;
  std::string user;
  std::string passwd;
  std::string host;
  int port;
} db_params;

db_params parse_db_conn_params();

namespace psql{

  void create_pk_constraint(const std::string& model_name, const std::vector<std::string>& pk_cols, std::ofstream& Migrations);

  void create_fk_constraint(const std::string& model_name, const std::string& fk_sql_segment,
                            const std::string& column_name, std::ofstream& Migrations);

  void create_uq_constraint(const std::string& uq_col, const std::ofstream& Migrations);

  void create_column(const std::string& column_name, const std::string& column_sql_attributes, std::ofstream& Migrations);

  template <typename Col_Map>
  void create_table(const std::string& model_name, Col_Map& fields, std::ofstream& Migrations);

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

  void drop_table(const std::string& model_name, std::ofstream& Migrations);

  void drop_column(const std::string& model_name, const std::string& column_name, std::ofstream& Migrations);

  void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations);

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

  #ifdef PSQL

  template<typename Model_T>
  void dbfetch(Model_T& obj, std::string& sql_string, bool getfn_called = false);

  namespace query{

    template <typename Model_T>
    void fetch_all(Model_T& obj);

    template <typename Model_T, typename... Args>
    void get(Model_T& obj, Args... args);

    template <typename Model_T, typename... Args>
    void filter(Model_T& obj, Args... args);

    template <typename Model_T, typename... Args>
    void select_related(Model_T& obj, Args... args);

    template <typename Model_T>
    std::vector<Model_T> to_instances(Model_T& obj);

    template <typename Model_T>
    std::vector<decltype(std::declval<Model_T>().get_attr())> to_values(Model_T& obj);

  }

  void execute_sql(std::string& file_name);

  #endif
}

#ifdef PSQL
  namespace db_adapter = psql;
#else
  #error "No valid db_engine specified"
#endif
