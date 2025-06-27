#include <string>
#include <vector>
#include <pqxx/row>
#include <tuple>
#include <fstream>
#include <strata/json.hpp>

class users{
public:
  std::string table_name = "users";
  int id;
  int pin;
  std::string email;
  std::string username;
  std::vector<pqxx::row> records;
  std::string col_str = "pin,email,username";
  int col_map_size = 3;

  users() = default;
  template <typename tuple_T>
  users(tuple_T tup){
    std::tie(id,pin,email,username) = tup;
  }

  auto get_attr() const{
    return std::make_tuple(id,pin,email,username);
  }

  std::vector<pqxx::params> parse_json_rows(){
    std::vector<pqxx::params> rows {};
    std::ifstream json_row_file("../../insert/users.json");
    if(!json_row_file.is_open()) std::runtime_error("Couldn't open file attempting to parse rows");

    nlohmann::json json_data = nlohmann::json::parse(json_row_file);

    for(auto& json_row: json_data){
      rows.push_back(pqxx::params{json_row["pin"].get<int>(),
                                  json_row["email"].get<std::string>(),
                                  json_row["username"].get<std::string>()
                                });
    }
    return rows;
  }
};

class message{
public:
  std::string table_name = "message";
  int id;
  std::string content;
  int receiver;
  int sender;
  std::vector<pqxx::row> records;
  std::string col_str = "content,receiver,sender";
  int col_map_size = 3;

  message() = default;
  template <typename tuple_T>
  message(tuple_T tup){
    std::tie(id,content,receiver,sender) = tup;
  }

  auto get_attr() const{
    return std::make_tuple(id,content,receiver,sender);
  }

  std::vector<pqxx::params> parse_json_rows(){
    std::vector<pqxx::params> rows {};
    std::ifstream json_row_file("../../insert/messages.json");
    if(!json_row_file.is_open()) std::runtime_error("Couldn't open file attempting to parse rows");

    nlohmann::json json_data = nlohmann::json::parse(json_row_file);

    for(auto& json_row: json_data){
      rows.push_back(pqxx::params{json_row["content"].get<std::string>(),
                                  json_row["receiver"].get<int>(),
                                  json_row["sender"].get<int>()
                                });
    }
    return rows;
  }
};

