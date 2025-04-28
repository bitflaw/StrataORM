#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "../orm++/db_engine_adapters.hpp"

std::string str_to_upper(std::string& str){
  for(char& ch: str){
    ch = std::toupper(ch);
  }
  return str;
}

db_params parse_db_conn_params(){
  std::ifstream dbconfigfile ("config.json");

  if(!dbconfigfile.is_open()) throw std::runtime_error("Could not load schema from file.");

  nlohmann::json j;
  dbconfigfile >> j;

  return db_params { j.at("db_name").get<std::string>(),
                     j.at("user").get<std::string>(),
                     j.at("passwd").get<std::string>(),
                     j.at("host").get<std::string>(),
                     j.at("port").get<int>()
                   };
}

namespace psql{

  void alter_rename_table(const std::string& old_model_name, const std::string& new_model_name, std::ofstream& Migrations){
    Migrations<< "ALTER TABLE " + old_model_name + " RENAME TO " + new_model_name + ";\n";
    return;
  }

  void alter_add_column(const std::string& model_name, const std::string& column_name,
                        const std::string& column_sql_attributes, std::ofstream& Migrations){
    Migrations<< "ALTER TABLE " + model_name + " ADD COLUMN " + column_name + " " + column_sql_attributes + ";\n";
    return;
  }

  void alter_rename_column(const std::string& model_name, const std::string& old_column_name,
                           const std::string& new_column_name, std::ofstream& Migrations){
    Migrations << "ALTER TABLE " + model_name + " RENAME COLUMN " + old_column_name + " TO " + new_column_name + ";\n";
    return;
  }

  void alter_column_type(const std::string& model_name, const std::string& column_name,
                         const std::string& sql_segment, std::ofstream& Migrations){
    Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " TYPE " + sql_segment + ";\n";
    return;
  }

  void alter_column_defaultval(const std::string& model_name, const std::string& column_name,
                               const bool set_default, const std::string& defaultval, std::ofstream& Migrations){
    if(set_default){
      Migrations << "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET DEFAULT " + defaultval + ";\n";
      return;
    }else{
      Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP DEFAULT;\n";
      return;
    }
  }

  void alter_column_nullable(const std::string& model_name, const std::string& column_name, const bool nullable, std::ofstream& Migrations){
    if(nullable){
      Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP NOT NULL;\n";
      return;
    }else{
      std::string default_value;
      std::cout<<"Provide a default value for the column '" + column_name +"' to be set to non-nullable: " << std::endl;
      std::cin>> default_value;
      Migrations<< "UPDATE " + model_name + " SET " + column_name + " = " + default_value + " WHERE " + column_name
                << " IS NULL; ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET NOT NULL;\n";
    }
  }

  void drop_table(const std::string& model_name, std::ofstream& Migrations){
    Migrations << "DROP TABLE " + model_name + ";\n";
    return;
  }

  void drop_column(const std::string& model_name, const std::string& column_name,  std::ofstream& Migrations){
    Migrations << "ALTER TABLE " + model_name + " DROP COLUMN " + column_name + ";\n";
    return;
  }

  void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations){
    Migrations<< "ALTER TABLE " + model_name + " DROP CONSTRAINT " + constraint_name + ";\n";
    return;
  }

  void generate_int_sql(IntegerField& int_obj){
	  int_obj.datatype = str_to_upper(int_obj.datatype);
		if(int_obj.datatype != "INTEGER" && int_obj.datatype != "SMALLINT" && int_obj.datatype != "BIGINT"){
			std::cerr<< "Datatype " <<int_obj.datatype<<" is not supported by postgreSQL. Provide a valid datatype"<<std::endl;
			return;
		}
		int_obj.sql_segment += int_obj.datatype;
		if (int_obj.not_null) int_obj.sql_segment = int_obj.datatype + " NOT NULL";
  }

  void generate_char_sql(CharField& char_obj){
    char_obj.datatype = str_to_upper(char_obj.datatype);
    if(char_obj.datatype != "VARCHAR" && char_obj.datatype != "CHAR" && char_obj.datatype != "TEXT"){
      std::cerr<< "Datatype " <<char_obj.datatype<<" is not supported by postgreSQL. Provide a valid datatype"<<std::endl;
      return;
    }
    char_obj.sql_segment += char_obj.datatype;
    if (char_obj.length == 0 && char_obj.datatype != "TEXT"){
      std::cerr<< "Length attribute is required for datatype " << char_obj.datatype << std::endl;
      return;
    }
    char_obj.sql_segment += "(" + std::to_string(char_obj.length) + ")";
    if (char_obj.not_null) char_obj.sql_segment +=  " NOT NULL";
  }

  void generate_decimal_sql(DecimalField& dec_obj){
    dec_obj.datatype = str_to_upper(dec_obj.datatype);
    if(dec_obj.datatype != "DECIMAL" && dec_obj.datatype != "REAL"&& 
        dec_obj.datatype != "DOUBLE PRECISION" && dec_obj.datatype != "NUMERIC"){
      std::cerr<< "Datatype" <<dec_obj.datatype<<"is not supported by postgreSQL. Provide a valid datatype"<<std::endl;
      return;
    }
    if(dec_obj.max_length > 0 || dec_obj.decimal_places > 0)
      dec_obj.sql_segment = dec_obj.datatype + "(" + std::to_string(dec_obj.max_length) 
                            + "," + std::to_string(dec_obj.decimal_places) + ")";
    else
      std::cerr<<"Max length and/or decimal places cannot be 0 for datatype " << dec_obj.datatype <<std::endl;
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
    if(bin_obj.size == 0){
        std::cerr<< "Size cannot be zero for datatype "<<bin_obj.datatype<<std::endl;
        return;
      }
    bin_obj.sql_segment = bin_obj.datatype + "(" + std::to_string(bin_obj.size) + ")";
    if(bin_obj.not_null) bin_obj.sql_segment += " NOT NULL";
  }

  void generate_datetime_sql(DateTimeField& dt_obj){
    dt_obj.datatype = str_to_upper(dt_obj.datatype);
		if(dt_obj.datatype != "DATE" && dt_obj.datatype != "TIME" && dt_obj.datatype != "TIMESTAMP_WTZ" &&
       dt_obj.datatype != "TIMESTAMP" && dt_obj.datatype != "TIME_WTZ"  && dt_obj.datatype != "INTERVAL"){
			std::cerr<<"Datatype " <<dt_obj.datatype<< "not supported in postgreSQL. Provide a valid datatype"<<std::endl;
			return;
		}

    std::string::size_type n = dt_obj.datatype.find('_');
    if(n != std::string::npos){
      dt_obj.datatype.replace(n+1, n+3, "WITH TIMEZONE");
    }

		dt_obj.sql_segment = dt_obj.datatype;
		if(dt_obj.enable_default && dt_obj.default_val != "default"){
			dt_obj.default_val = str_to_upper(dt_obj.default_val);
			dt_obj.sql_segment += " DEFAULT " + dt_obj.default_val;
		}
  }

  void generate_foreignkey_sql(ForeignKey& fk_obj){
    fk_obj.sql_segment ="FOREIGN KEY(" + fk_obj.col_name + ") REFERENCES " + fk_obj.model_name + " (" + fk_obj.ref_col_name + ")";

		if(fk_obj.on_delete != "def"){
			fk_obj.on_delete = str_to_upper(fk_obj.on_delete);
      fk_obj.sql_segment += " ON DELETE " + fk_obj.on_delete;
    }
		if(fk_obj.on_update != "def" ){
      fk_obj.on_update = str_to_upper(fk_obj.on_update);
      fk_obj.sql_segment += " ON UPDATE " + fk_obj.on_update;
	  }
  }

  void execute_sql(std::string& file_name){
    std::ifstream sql_file(file_name);

    std::ostringstream raw_sql;
    raw_sql << sql_file.rdbuf();

    db_params params = parse_db_conn_params();
    try{
      pqxx::connection cxn("dbname=" + params.db_name+
                            " user=" + params.user +
                            " password=" + params.passwd +
                            " host=" + params.host +
                            " port=" + std::to_string(params.port)
                            );
      pqxx::work txn(cxn);

      std::cout<<"If you would like to view the sql changes, view them in the " + file_name + " file in the file tree. DO NOT CLOSE THIS!\n"
                <<"To continue, enter (y)es to execute the sql, else, enter (n)o to abort the sql execution. :"<<std::endl;

      char choice = 'y';
      std::cin >> choice;

      if(choice == 'n'){
        return;
      }

      txn.exec0(raw_sql.str());

      txn.commit();
    }catch (const std::exception& e){
      throw std::runtime_error(e.what());
    }
    return;
  }

}
