#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <variant>
#include "../includes/datatypes.hpp"
#include "../includes/db_engine_adapters.hpp"

#ifdef PSQL
  #include <pqxx/pqxx>
#else
  #error "No other thing is defined"
#endif

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
        if constexpr(std::is_same_v<std::decay_t<decltype(col_obj)>, ForeignKey>){
          create_column(col_obj.column_name,col_obj.sql_type, Migrations);
          Migrations << ",";
          create_fk_constraint(model_name, col_obj.sql_segment, col_obj.column_name, Migrations);
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
	  int_obj.datatype = str_to_upper(int_obj.datatype);
		if(int_obj.datatype != "INT" && int_obj.datatype != "SMALLINT" && int_obj.datatype != "BIGINT"){
			std::cerr<< "Datatype " <<int_obj.datatype<<" is not supported by postgreSQL. Provide a valid datatype"<<std::endl;
			return;
		}
		int_obj.sql_segment += int_obj.datatype;
		if (int_obj.not_null) int_obj.sql_segment = int_obj.datatype + " NOT NULL";
  }

  template <typename CharType>
  void generate_char_sql(CharType& char_obj){
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

  template <typename DecimalType>
  void generate_decimal_sql(DecimalType& dec_obj){
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

  template <typename FKType>
  void generate_foreignkey_sql(FKType& fk_obj){
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

  #ifdef PSQL

  template<typename Model_T>
  void dbfetch(Model_T& obj, std::string& sql_string, bool getfn_called){
    db_params params = parse_db_conn_params();
    try{
      pqxx::connection cxn("dbname=" + params.db_name+
                           " user=" + params.user +
                           " password=" + params.passwd +
                           " host=" + params.host +
                           " port=" + std::to_string(params.port)
                           );
      pqxx::work txn(cxn);

      if(getfn_called){
        obj.records.push_back(txn.exec1(sql_string));
        return;
      }

      for(const pqxx::row& row : txn.exec(sql_string)){
        obj.records.push_back(row);
      }

      txn.commit();
    }catch (const std::exception& e){
      throw std::runtime_error(e.what());
    }
    return;
  }

  template <typename Arg>
  std::string to_str(const Arg& arg){
    std::ostringstream ss;
    ss<<arg;
    return  ss.str();
  }

  template <typename... Args>
  std::vector<std::string> make_arg_vec(Args&&... args){
    return {to_str(args)...};
  }

  namespace query{

    template <typename Model_T>
    void fetch_all(Model_T& obj, std::string& model_name){
      std::string sql_string = "select * from " + model_name + ";";
      dbfetch(obj, sql_string);
    }

    template <typename Model_T, typename... Args>
    void get(Model_T& obj, std::string& model_name, Args... args){
      if(sizeof...(args) == 0 || sizeof...(args)%2 != 0){
        throw std::runtime_error("Args are provided in key-value pairs. Please check if you have missed either a key or a value!");
      }else{
        std::string sql_kwargs;
        std::vector<std::string>args = make_arg_vec(std::forward<Args>(args)...);
        std::vector<std::pair<std::string, std::string>> kwargs;
        for(int i = 0; i < args.size(); i+=2){
          sql_kwargs += args[i] + "=" + args[i+1] + " and ";
          kwargs.push_back(std::make_pair(args[i], args[i+1]));
        }

        sql_kwargs.replace(sql_kwargs.size()-5, 5, ";");

        if(obj.records.empty()){
          std::string sql_str = "select * from " + model_name + "where " + sql_kwargs;
          dbfetch(obj, sql_str, true);
          return;
        }else{
          std::vector<pqxx::row> filtered_rows;
          for(const pqxx::row& row : obj.records){
            bool accept_row = true;
            for(const auto& kwarg : kwargs){
              if(row[kwarg.first].template as<std::string>() != kwarg.second){
                accept_row = false;
                break;
              }
            }
            if(accept_row){
              filtered_rows.push_back(row);
              continue;
            }
          }
          obj.records = filtered_rows;
          return;
        }
      }
    }

    template <typename Model_T, typename... Args>
    void filter(Model_T& obj, std::string& model_name, Args... args){
      if(sizeof...(args) == 0){
        throw std::runtime_error("Filter parameters are required for the .filter() function.");
      }else{
        std::vector<std::string>args = make_arg_vec(std::forward<Args>(args)...);
        std::vector<std::pair<std::string, std::pair<std::string, std::string>>> kwargs;

        for(int i = 0; i < args.size(); i+=2){
          std::string::size_type n = args[i].find("__");
          if(std::string::npos != n){
            kwargs.push_back(std::make_pair(args[i].substr(n+2), std::make_pair(args[i].substr(0, n-1), args[i+1])));
          }else{
            throw std::runtime_error("A filter constraint is required in the form \'__constraint\' after the column name you want to filter.");
          }
        }

        if(obj.records.empty()){
          std::string sql_str = "select * from " + model_name + " where ";

          for(const auto& pair: kwargs){
            if(pair.first == "startswith"){
              sql_str += pair.second.first + " like '" + pair.second.second + "%' and ";
            }else if(pair.first == "endswith"){
              sql_str += pair.second.first + " like '%" + pair.second.second + "' and ";
            }else if(pair.first == "gte"){
              sql_str += pair.second.first + " >= " + pair.second.second + " and ";
            }else if(pair.first == "lte"){
              sql_str += pair.second.first + " <= " + pair.second.second + "' and ";
            }else if(pair.first == "gt"){
              sql_str += pair.second.first + " > " + pair.second.second + "' and ";
            }else if(pair.first == "lt"){
              sql_str += pair.second.first + " < " + pair.second.second + "' and ";
            }else if(pair.first == "contains"){
              sql_str += pair.second.first + " like '" + pair.second.second + "%' and ";
            }else if(pair.first == "betweenNAN"){
              sql_str += pair.second.first + " between '" + pair.second.second + "' and ";
            }else if(pair.first == "betweenN"){
              sql_str += pair.second.first + " between " + pair.second.second + " and ";
            }else if(pair.first == "eql"){
              sql_str += pair.second.first + " = " + pair.second.second + " and ";
            }else{
              throw std::runtime_error("The constraint specified is not implemented yet or is not correct.");
            }
          }
          sql_str.replace(sql_str.size()-5, 5, ";");
          dbfetch(obj, sql_str);
          return;
        }else{
          std::vector<pqxx::row> filtered;
          for(const pqxx::row& row : obj.records){
            bool accept_row = false;
            for(const auto& pair : kwargs){
              if(pair.first == "startswith"){
                std::string pattern = row[pair.second.first].template as<std::string>().substr(pair.second.second.size()-1);
                if(pattern != pair.second.second){
                  accept_row = false;
                  break;
                }
              }else if(pair.first == "endswith"){
                int start_pos = row[pair.second.first].size();
                std::string pattern = row[pair.second.first].template as<std::string>().substr(start_pos - pair.second.second.size());
                if(pattern != pair.second.second){
                  accept_row = false;
                  break;
                }
              }else if(pair.first == "gte"){
                float col_value = row[pair.second.first].template as<float>();
                if(col_value < std::stof(pair.second.second)){
                  accept_row = false;
                  return;
                }
              }else if(pair.first == "lte"){
                float col_value = row[pair.second.first].template as<float>();
                if(col_value > std::stof(pair.second.second)){
                  accept_row = false;
                  return;
                }
              }else if(pair.first == "gt"){
                float col_value = row[pair.second.first].template as<float>();
                if(col_value < std::stof(pair.second.second)){
                  accept_row = false;
                  return;
                }
              }else if(pair.first == "lt"){
                float col_value = row[pair.second.first].template as<float>();
                if(col_value > std::stof(pair.second.second)){
                  accept_row = false;
                  return;
                }
              }else if(pair.first == "contains"){
                size_t index_for_substr = row[pair.second.first].template as<std::string>().find(pair.second.second);
                if(index_for_substr == std::string::npos){
                  accept_row = false;
                  break;
                }
              }else if(pair.first == "eql"){
                if(row[pair.second.first].template as<std::string>() != pair.second.second){
                  accept_row = false;
                  return;
                }
              }else{
                throw std::runtime_error("The constraint specified is not implemented yet or is not correct.");
              }
            }

            if(accept_row){
              filtered.push_back(row);
              continue;
            }
          }
          obj.records = filtered;
          return;
        }
      }
    }

    //template <typename Model_T, typename... Args>
    //  void select_related(Model_T& obj, Args... args){}//NOTE to be implemented later, soln not found yet

    template <typename Model_T>
    std::vector<Model_T> to_instances(Model_T& obj){
      using tuple_T = decltype(obj.get_attr());
      std::vector<Model_T> instances;

      for(const pqxx::row& row : obj.records){
        instances.push_back(Model_T(row.template as_tuple<tuple_T>()));
      }

      return instances;
    }

    template <typename Model_T>
    std::vector<decltype(std::declval<Model_T>().get_attr())> to_values(Model_T& obj){
      using tuple_T = decltype(obj.get_attr());
      std::vector<tuple_T> values;

      for(const pqxx::row& row : obj.records){
        values.push_back(row.template as_tuple<tuple_T>());
      }

      return values;
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
  #endif
}
