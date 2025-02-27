#include "../includes/dbquery.hpp"
#include <stdexcept>
#include <string>
#include <utility>

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
    std::cerr<<"Args are provided in key-value pairs. Please check if you have missed either a key or a value!"<<std::endl;
    return;
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
    std::cerr<<"Filter parameters are required for the .filter() function."<<std::endl;
    return;
  }else{
    std::vector<std::string>args = make_arg_vec(std::forward<Args>(args)...);
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> kwargs;

    for(int i = 0; i < args.size(); i+=2){
      std::string::size_type n = args[i].find("__");
      if(std::string::npos != n){
        kwargs.push_back(std::make_pair(args[i].substr(n+2), std::make_pair(args[i].substr(0, n-1), args[i+1])));
      }else{
        kwargs.push_back(std::make_pair("null", std::make_pair(args[i], args[i+1])));
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
        }else if(pair.first == "null"){
          sql_str += pair.second.first + " = " + pair.second.second + " and ";
        }else{
          std::runtime_error("The constraint specified is not implemented yet or is not correct.");
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
          }else if(pair.first == "null"){
            if(row[pair.second.first].template as<std::string>() != pair.second.second){
              accept_row = false;
              return;
            }
          }else{
            std::runtime_error("The constraint specified is not implemented yet or is not correct.");
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

/*template <typename Model_T, typename... Args>
  void select_related(Model_T& obj, Args... args){}*///NOTE to be implemented later, soln not found yet

//TODO insert fn which utilizes the dbinsert fn in the dbclient.hpp to insert values into the db tables

template <typename Model_T>
std::vector<Model_T> to_instances(Model_T& obj){
  std::vector<Model_T> instances;
  for(const pqxx::row& row : obj.records){
    instances.push_back(Model_T(row.template as_tuple<decltype(obj.get_attr())>()));
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
