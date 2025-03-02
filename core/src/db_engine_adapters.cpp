#include "../includes/db_engine_adapters.hpp"
#include <iostream>

std::string to_upper(std::string& str){
  for(char& ch : str){
    ch = toupper(ch);
  }
  return str;
}

namespace psql{

  template <typename Col_Map>
  void create_table(std::string& model_name, Col_Map& fields, std::ofstream& Migrations){}

  template <typename IntType>
  void generate_int_sql(IntType& int_obj){
		int_obj.datatype = to_upper(int_obj.datatype);
		if(int_obj.datatype != "INT" && int_obj.datatype != "SMALLINT" && int_obj.datatype != "BIGINT"){
			std::cerr<< "Datatype " <<int_obj.datatype<<" is not supported by postgreSQL. Provide a valid datatype"<<std::endl;
			return;
		}
		int_obj.sql_segment += int_obj.datatype;
		if (int_obj.not_null) int_obj.sql_segment = int_obj.datatype + " NOT NULL";
  }

  template <typename CharType>
  void generate_char_sql(CharType& char_obj){
    char_obj.datatype = to_upper(char_obj.datatype);
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

  template <typename DecimalType>
  void generate_decimal_sql(DecimalType& dec_obj){
    dec_obj.datatype = to_upper(dec_obj.datatype);
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

  template <typename BoolType>
  void generate_bool_sql(BoolType& bool_obj){
    bool_obj.sql_segment += bool_obj.datatype;
    if (bool_obj.not_null)bool_obj.sql_segment += " NOT NULL";
    if (bool_obj.enable_default){
      if(bool_obj.default_value)bool_obj.sql_segment += " DEFAULT TRUE";
      else bool_obj.sql_segment += " DEFAULT FALSE";
    }
  }

  template <typename BinType>
  void generate_bin_sql(BinType& bin_obj){
    if(bin_obj.size == 0){
        std::cerr<< "Size cannot be zero for datatype "<<bin_obj.datatype<<std::endl;
        return;
      }
    bin_obj.sql_segment = bin_obj.datatype + "(" + std::to_string(bin_obj.size) + ")";
    if(bin_obj.not_null) bin_obj.sql_segment += " NOT NULL";
  }

  template <typename DTType>
  void generate_datetime_sql(DTType& dt_obj){
    dt_obj.datatype = to_upper(dt_obj.datatype);
		if(dt_obj.datatype != "DATE" && dt_obj.datatype != "TIME" && dt_obj.datatype != "TIMESTAMP_WTZ" &&
       dt_obj.datatype != "TIMESTAMP" && dt_obj.datatype != "TIME_WTZ"  && dt_obj.datatype != "INTERVAL"){
			std::cerr<<"Datatype " <<dt_obj.datatype<< "not supported in postgreSQL. Provide a valid datatype"<<std::endl;
			return;
		}

		dt_obj.sql_segment = dt_obj.datatype;
		if(dt_obj.enable_default && dt_obj.default_val != "default"){
			dt_obj.default_val = to_upper(dt_obj.default_val);
			dt_obj.sql_segment += " DEFAULT " + dt_obj.default_val;
		}
  }

  template <typename FKType>
  void generate_foreignkey_sql(FKType& fk_obj){
    if(fk_obj.model_name == "def" || fk_obj.ref_col_name == "def"){
      std::cerr<< "Please provide the reference model name and column name for the column "<< fk_obj.col_name << std::endl;
		}
	  fk_obj.sql_segment ="FOREIGN KEY(" + fk_obj.col_name + ") REFERENCES " + fk_obj.model_name + " (" + fk_obj.ref_col_name + ")";

		if(fk_obj.on_delete != "def"){
			fk_obj.on_delete = to_upper(fk_obj.on_delete);
			fk_obj.sql_segment += " ON DELETE " + fk_obj.on_delete;
		} 
		if(fk_obj.on_update != "def" ){
			fk_obj.on_update = to_upper(fk_obj.on_update);
			fk_obj.sql_segment += " ON UPDATE " + fk_obj.on_update;
		}
  }

}
