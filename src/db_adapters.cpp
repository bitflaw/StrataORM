#include <cctype>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "../strata/db_adapters.hpp"

std::string str_to_upper(std::string& str){
  for(char& ch: str){
    ch = std::toupper(ch);
  }
  return str;
}

Utils::db_params Utils::parse_db_conn_params(){
  std::ifstream dbconfigfile ("config.json");

  if(!dbconfigfile.is_open()) throw std::runtime_error("[ERROR: in 'parse_db_conn_params()']Could not load db params from config.json.");

  nlohmann::json j;
  dbconfigfile >> j;
//NOTE intro a try catch here for more informative error handling
  return Utils::db_params { j.at("db_name").get<std::string>(),
                     j.at("user").get<std::string>(),
                     j.at("passwd").get<std::string>(),
                     j.at("host").get<std::string>(),
                     j.at("port").get<int>()
                   };
}

namespace psql{

void alter_rename_table(const std::string& old_model_name, const std::string& new_model_name, std::ofstream& Migrations){
  Migrations<< "ALTER TABLE " + old_model_name + " RENAME TO " + new_model_name + ";\n";
}

void alter_add_column(const std::string& model_name, const std::string& column_name,
                      const std::string& column_sql_attributes, std::ofstream& Migrations){
  Migrations<< "ALTER TABLE " + model_name + " ADD COLUMN " + column_name + " " + column_sql_attributes + ";\n";
}

void alter_rename_column(const std::string& model_name, const std::string& old_column_name,
                         const std::string& new_column_name, std::ofstream& Migrations){
  Migrations << "ALTER TABLE " + model_name + " RENAME COLUMN " + old_column_name + " TO " + new_column_name + ";\n";
}

void alter_column_type(const std::string& model_name, const std::string& column_name,
                       const std::string& sql_segment, std::ofstream& Migrations){
  Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " TYPE " + sql_segment + ";\n";
}

void alter_column_defaultval(const std::string& model_name, const std::string& column_name,
                             const bool set_default, const std::string& defaultval, std::ofstream& Migrations){
  if(set_default){
    Migrations << "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET DEFAULT " + defaultval + ";\n";
  }else{
    Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP DEFAULT;\n";
  }
}

void alter_column_nullable(const std::string& model_name, const std::string& column_name, const bool nullable, std::ofstream& Migrations){
  if(nullable){
    Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP NOT NULL;\n";
  }else{
    std::string default_value;
    std::cout<<"Provide a default value for the column '" + column_name +"' to be set to non-nullable: " << std::endl;
    std::cin>> default_value;
    Migrations<< std::boolalpha
      << "UPDATE " + model_name + " SET " + column_name + " = " + default_value + " WHERE " + column_name
      << " IS NULL;\n ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET NOT NULL;\n";
  }
}

void drop_table(const std::string& model_name, std::ofstream& Migrations){
  Migrations << "DROP TABLE IF EXISTS " + model_name + ";\n";
}

void drop_column(const std::string& model_name, const std::string& column_name,  std::ofstream& Migrations){
  Migrations << "ALTER TABLE " + model_name + " DROP COLUMN " + column_name + ";\n";
}

void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations){
  Migrations<< "ALTER TABLE " + model_name + " DROP CONSTRAINT " + constraint_name + ";\n";
}

void generate_int_sql(IntegerField& int_obj){
  int_obj.datatype = str_to_upper(int_obj.datatype);
  if(int_obj.datatype != "INTEGER" && int_obj.datatype != "SMALLINT" && int_obj.datatype != "BIGINT"){
    throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL. Provide a valid datatype", int_obj.datatype));
  }
  int_obj.sql_segment += int_obj.datatype;
  if (int_obj.not_null) int_obj.sql_segment = int_obj.datatype + " NOT NULL";
}

void generate_char_sql(CharField& char_obj){
  char_obj.datatype = str_to_upper(char_obj.datatype);
  if(char_obj.datatype != "VARCHAR" && char_obj.datatype != "CHAR" && char_obj.datatype != "TEXT"){
    throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL. Provide a valid datatype", char_obj.datatype));
  }
  char_obj.sql_segment += char_obj.datatype;
  if (char_obj.length == 0 && char_obj.datatype != "TEXT"){
    throw std::runtime_error(std::format("Length attribute is required for datatype '{}'", char_obj.datatype));
  }
  char_obj.sql_segment += "(" + std::to_string(char_obj.length) + ")";
  if (char_obj.not_null) char_obj.sql_segment +=  " NOT NULL";
}

void generate_decimal_sql(DecimalField& dec_obj){
  dec_obj.datatype = str_to_upper(dec_obj.datatype);

  if(dec_obj.datatype != "DECIMAL" && dec_obj.datatype != "REAL" &&
    dec_obj.datatype != "DOUBLE PRECISION" && dec_obj.datatype != "NUMERIC"){
    throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL. Provide a valid datatype", dec_obj.datatype));
  }

  if(dec_obj.datatype == "REAL" || dec_obj.datatype == "DOUBLE PRECISION"){
    dec_obj.sql_segment = dec_obj.datatype;
    return;
  }

  if(dec_obj.max_length > 0 || dec_obj.decimal_places > 0)
    dec_obj.sql_segment = dec_obj.datatype + "(" + std::to_string(dec_obj.max_length) + "," + std::to_string(dec_obj.decimal_places) + ")";
  else
    throw std::runtime_error(std::format("Max length and/or decimal places cannot be 0 for datatype '{}'", dec_obj.datatype));
}

void generate_bool_sql(BoolField& bool_obj){
  bool_obj.sql_segment += bool_obj.datatype;
  if (bool_obj.not_null)bool_obj.sql_segment += " NOT NULL";
  if (bool_obj.enable_default){
    if(bool_obj.default_value)bool_obj.sql_segment += " DEFAULT TRUE";
    else bool_obj.sql_segment += " DEFAULT FALSE";
  }
}

void generate_bin_sql(BinaryField& bin_obj){
  bin_obj.sql_segment = bin_obj.datatype;
  if(bin_obj.not_null) bin_obj.sql_segment += " NOT NULL";
}

void generate_datetime_sql(DateTimeField& dt_obj){
  dt_obj.datatype = str_to_upper(dt_obj.datatype);
  if(dt_obj.datatype != "DATE" && dt_obj.datatype != "TIME" && dt_obj.datatype != "TIMESTAMP_WTZ" &&
    dt_obj.datatype != "TIMESTAMP" && dt_obj.datatype != "TIME_WTZ"  && dt_obj.datatype != "INTERVAL"){
    throw std::runtime_error(std::format("Datatype '{}' not supported in postgreSQL. Provide a valid datatype", dt_obj.datatype));
    return;
  }

  std::string::size_type n = dt_obj.datatype.find('_');
  if(n != std::string::npos){
    dt_obj.datatype.replace(n+1, n+3, "WITH TIME ZONE");
  }

  dt_obj.sql_segment = dt_obj.datatype;
  if(dt_obj.enable_default && !dt_obj.default_val.empty()){
    dt_obj.default_val = str_to_upper(dt_obj.default_val);
    dt_obj.sql_segment += " DEFAULT " + dt_obj.default_val;
  }
}

void generate_foreignkey_sql(ForeignKey& fk_obj){
  fk_obj.sql_segment ="FOREIGN KEY(" + fk_obj.col_name + ") REFERENCES " + fk_obj.model_name + " (" + fk_obj.ref_col_name + ")";
  fk_obj.on_delete = str_to_upper(fk_obj.on_delete);
  fk_obj.sql_segment += " ON DELETE " + fk_obj.on_delete;
  fk_obj.on_update = str_to_upper(fk_obj.on_update);
  fk_obj.sql_segment += " ON UPDATE " + fk_obj.on_update;
}

void create_models_hpp(const ms_map& migrations){
  std::ofstream models_hpp("models.hpp");
  std::string cols_str {};

  if(!migrations.empty())
    models_hpp<<"#include <string>\n#include <vector>\n"
              <<"#include <pqxx/row>\n#include <tuple>\n\n";

  for(const auto& [model_name, col_map] : migrations){
    models_hpp<<"class " + model_name + "{\npublic:\n"
              <<"  std::string table_name = \"" + model_name + "\";\n  int id;\n";
    for(const auto& [col_name, dtv_obj] : col_map){
      cols_str += col_name + ",";
      std::visit([&](auto& col_obj){
        models_hpp<< "  " + col_obj->ctype + " " + col_name + ";\n";
      }, dtv_obj);
    }
    cols_str.pop_back();
    models_hpp<< "  std::vector<pqxx::row> records;\n"
      << "  std::string col_str = \"" + cols_str + "\";\n"
      << "  int col_map_size = " + std::to_string(col_map.size()) + ";\n\n"
      << "  " + model_name + "() = default;\n"
      << "  template <typename tuple_T>\n"
      << "  " + model_name + "(tuple_T tup){\n"
      << "    std::tie(id," + cols_str + ") = tup;\n  }\n\n"
      << "  auto get_attr() const{\n"
      << "    return std::make_tuple(id," + cols_str + ");\n  }\n};\n\n";
    cols_str.clear();
  }
}

void exec_insert(pqxx::connection& cxn, pqxx::params& row){
  try{
    pqxx::work txn(cxn);
    pqxx::result result = txn.exec(pqxx::prepped{"insert_stmt"}, row).no_rows();
    txn.commit();
  }catch(const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'exec_insert()'] => {}.", e.what()));
  }
}

std::optional<pqxx::result> execute_sql(std::string& sql_file_or_str, bool is_file_name){
  std::ostringstream raw_sql {};

  if(is_file_name){
    std::ifstream sql_file(sql_file_or_str);
    if(!sql_file.is_open()) throw std::runtime_error("[ERROR: in 'execute_sql()'] => Couldn't open the sql file to which the path is provided.");
    raw_sql << sql_file.rdbuf();
  }else{
    raw_sql << sql_file_or_str;
  }

  Utils::db_params params = Utils::parse_db_conn_params();

  try{
    pqxx::connection cxn("dbname=" + params.db_name+
                         " user=" + params.user +
                         " password=" + params.passwd +
                         " host=" + params.host +
                         " port=" + std::to_string(params.port)
                         );
    pqxx::work txn(cxn);

    pqxx::result results = txn.exec(raw_sql.str());

    txn.commit();

    if(!results.empty()) return results;
    return std::nullopt;
  }catch (const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'execute_sql()'] => Failed to execute sql. {}", e.what()));
  }
}

}
