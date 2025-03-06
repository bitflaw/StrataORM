#include <iostream>
#include <vector>
#include <variant>
#include "../includes/datatypes.hpp"
#include "../includes/db_engine_adapters.hpp"

std::string to_upper(std::string& str){
  for(char& ch : str){
    ch = toupper(ch);
  }
  return str;
}

namespace psql{

  void create_pk_constraint(const std::string& model_name, const std::vector<std::string>& pk_cols, std::ofstream& Migrations){
    std::string pk_seg = "CONSTRAINT pk_" + model_name + " PRIMARY KEY (" + model_name + "_id)";
    if (!pk_cols.empty()) {
      pk_seg.replace(pk_seg.length() - 1, 1, ",");
      for(const auto& col : pk_cols) {
        pk_seg += col + ",";
      }
      pk_seg.replace(pk_seg.length() - 1, 1, ")");
    }
    Migrations<< pk_seg;
    return;
  }

  void create_fk_constraint(const std::string& model_name, const std::string& fk_sql_segment,
                            const std::string& column_name, std::ofstream& Migrations){
    Migrations<< "CONSTRAINT fk_" + model_name + " " + fk_sql_segment;
    return;
  }

  void create_uq_constraint(const std::string& uq_col, std::ofstream& Migrations){
    Migrations<<"CONSTRAINT uq_" + uq_col + " UNIQUE (" + uq_col + "),";
    return;
  }

  void create_column(const std::string& column_name, const std::string& column_sql_attributes, std::ofstream& Migrations){
    Migrations<< column_name + " " + column_sql_attributes;
    return;
  }

  template <typename Col_Map>
  void create_table(const std::string& model_name, Col_Map& field_map, std::ofstream& Migrations){
    std::vector<std::string> primary_key_cols;
    std::vector<std::string> unique_constraint_cols;

    Migrations<< "CREATE TABLE " + model_name + " (" + model_name + "_id SERIAL NOT NULL, ";

    for(auto& [col, dtv_obj] : field_map){
      std::visit([&](auto& col_obj){
        if constexpr(std::is_same_v<std::decay_t<decltype(col_obj)>, ForeignKey>){//TODO fk col type must be same as the pk it i
        //is referencing, so check accordingly, maybe check field map to find the specific pk column
          //create_column(col_obj.referenced_column, Migrations);
          //Migrations << ",";
          create_fk_constraint(model_name, col, Migrations);
          return;
        }
        if(col_obj.primary_key){
          primary_key_cols.push_back(col);
        }

        if(col_obj.unique){
          unique_constraint_cols.push_back(col);
        }

        create_column(col, col.sql_segment, Migrations);
        Migrations << ",";
      }, dtv_obj);
    }

    for(std::string& col: unique_constraint_cols) create_uq_constraint(col, Migrations);
    create_pk_constraint(model_name, primary_key_cols, Migrations);
    Migrations<< ");";
    return;
  }

  void alter_rename_table(const std::string& old_model_name, const std::string& new_model_name, std::ofstream& Migrations){
    Migrations<< "ALTER TABLE " + old_model_name + " RENAME TO " + new_model_name + ";";
    return;
  }

  void alter_add_column(const std::string& model_name, const std::string& column_name,
                        const std::string& column_sql_attributes, std::ofstream& Migrations){
    Migrations<< "ALTER TABLE " + model_name + " ADD COLUMN " + column_name + " " + column_sql_attributes + ";";
    return;
  }

  void alter_rename_column(const std::string& model_name, const std::string& old_column_name,
                           const std::string& new_column_name, std::ofstream& Migrations){
    Migrations << "ALTER TABLE " + model_name + " RENAME COLUMN " + old_column_name + " TO " + new_column_name + ";";
    return;
  }

  void alter_column_type(const std::string& model_name, const std::string& column_name,
                         const std::string& sql_segment, std::ofstream& Migrations){
    Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " TYPE " + sql_segment + ";";
    return;
  }

  void alter_column_defaultval(const std::string& model_name, const std::string& column_name,
                               const bool set_default, const std::string& defaultval, std::ofstream& Migrations){
    if(set_default){
      Migrations << "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET DEFAULT " + defaultval + ";";
      return;
    }else{
      Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP DEFAULT;";
      return;
    }
  }

  void alter_column_nullable(const std::string& model_name, const std::string& column_name, const bool nullable, std::ofstream& Migrations){
    if(nullable){
      Migrations<< "ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " DROP NOT NULL;";
      return;
    }else{
      std::string default_value;
      std::cout<<"Provide a default value for the column '" + column_name +"' to be set to non-nullable: " << std::endl;
      std::cin>> default_value;
      Migrations<< "UPDATE " + model_name + " SET " + column_name + " = " + default_value + " WHERE " + column_name
                << " IS NULL; ALTER TABLE " + model_name + " ALTER COLUMN " + column_name + " SET NOT NULL;";
    }
  }

  void drop_table(const std::string& model_name, std::ofstream& Migrations){
    Migrations << "DROP TABLE " + model_name + ";";
    return;
  }

  void drop_column(const std::string& model_name, const std::string& column_name,  std::ofstream& Migrations){
    Migrations << "ALTER TABLE " + model_name + " DROP COLUMN " + column_name + ";";
    return;
  }

  void drop_constraint(const std::string& model_name, const std::string& constraint_name, std::ofstream& Migrations){
    Migrations<< " ALTER TABLE " + model_name + " DROP CONSTRAINT " + constraint_name + ";";
    return;
  }

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
