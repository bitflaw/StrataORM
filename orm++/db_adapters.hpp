#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "./db_config.hpp"
#include "./datatypes.hpp"
#include "./models.hpp"

typedef struct{
  std::string db_name;
  std::string user;
  std::string passwd;
  std::string host;
  int port;
} db_params;

db_params parse_db_conn_params();

#ifdef PSQL

#include <pqxx/pqxx>

namespace psql{

inline void create_pk_constraint(const std::string& model_name, const std::vector<std::string>& pk_cols, std::ofstream& Migrations){
  std::string pk_seg = "CONSTRAINT pk_" + model_name + " PRIMARY KEY (" + model_name + "_id)";
  if (!pk_cols.empty()) {
    pk_seg.replace(pk_seg.length() - 1, 1, ",");
    for(const auto& col : pk_cols) {
      pk_seg += col + ",";
    }
    pk_seg.replace(pk_seg.length() - 1, 1, ")");
  }
  Migrations<<pk_seg;
  return;
}

inline void create_fk_constraint(const std::string& model_name, const std::string& fk_sql_segment,
                                 const std::string& column_name, std::ofstream& Migrations){
  Migrations<< "CONSTRAINT fk_" + column_name + " " + fk_sql_segment;
  return;
}

inline void create_uq_constraint(const std::string& uq_col, std::ofstream& Migrations){
  Migrations<<"CONSTRAINT uq_" + uq_col + " UNIQUE (" + uq_col + "),\n";
  return;
}

inline void create_column(const std::string& column_name, const std::string& column_sql_attributes, std::ofstream& Migrations){
  Migrations<<column_name + " " + column_sql_attributes;
  return;
}

template <typename Col_Map>
void create_table(const std::string& model_name, Col_Map& field_map, std::ofstream& Migrations){
  std::vector<std::string> primary_key_cols;
  std::vector<std::string> unique_constraint_cols;

  Migrations<< "CREATE TABLE " + model_name + " (\n  " + model_name + "_id SERIAL NOT NULL,\n  ";

  for(auto& [col, dtv_obj] : field_map){
    std::visit([&](auto& col_obj){
      if constexpr(std::is_same_v<std::decay_t<decltype(col_obj)>, std::shared_ptr<ForeignKey>>){
        Migrations << "  ";
        create_column(col, col_obj->sql_type, Migrations);
        Migrations << ",\n  ";
        create_fk_constraint(model_name, col_obj->sql_segment, col, Migrations);
        Migrations << ",\n  ";
        return;
      }
      if(col_obj->primary_key){
        primary_key_cols.push_back(col);
      }

      if(col_obj->unique){
        unique_constraint_cols.push_back(col);
      }

      create_column(col, col_obj->sql_segment, Migrations);
      Migrations << ",\n  ";
    }, dtv_obj);
  }

  for(std::string& col: unique_constraint_cols){
    Migrations << "  ";
    create_uq_constraint(col, Migrations);
  }

  Migrations << "  ";
  create_pk_constraint(model_name, primary_key_cols, Migrations);
  Migrations<< "\n);\n\n";

  return;
}

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

void generate_int_sql(IntegerField& int_obj);

void generate_char_sql(CharField& char_obj);

void generate_decimal_sql(DecimalField& dec_obj);

void generate_bool_sql(BoolField& bool_obj);

void generate_bin_sql(BinaryField& bin_obj);

void generate_datetime_sql(DateTimeField& dt_obj);

void generate_foreignkey_sql(ForeignKey& fk_obj);

void create_models_hpp(const ms_map& migrations);

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

    for(const pqxx::row& row : txn.exec(sql_string)) obj.records.push_back(row);

    txn.commit();
  }catch (const std::exception& e){
    throw std::runtime_error(R"([ERROR: in 'db_fetch()'] => Couldn't complete database operations.)");
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
    throw std::runtime_error(R"([ERROR: in 'db_adapter::query::get()'] => 
                             Args are provided in key-value pairs. Please check if you have missed either a key or a value!)");
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
    throw std::runtime_error("[ERROR: in 'db_adapter::query::filter()'] => Filter parameters are required for the .filter() function.");
  }else{
    std::vector<std::string>args = make_arg_vec(std::forward<Args>(args)...);
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> kwargs;

    for(int i = 0; i < args.size(); i+=2){
      std::string::size_type n = args[i].find("__");
      if(std::string::npos != n){
        kwargs.push_back(std::make_pair(args[i].substr(n+2), std::make_pair(args[i].substr(0, n-1), args[i+1])));
      }else{
        throw std::runtime_error(R"([ERROR: in 'filter()'] => 
                                 A filter constraint is required in the form '__constraint' after the column name you want to filter.)");
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
          throw std::runtime_error("[ERROR: in 'filter()'] => The constraint specified is not implemented yet or is not correct.");
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
            throw std::runtime_error("[ERROR: in 'filter()'] => The constraint specified is not implemented yet or is not correct.");
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
void execute_sql(std::string& file_name);

}
namespace db_adapter = psql;

#else

#error "No valid db_engine specified"

#endif
