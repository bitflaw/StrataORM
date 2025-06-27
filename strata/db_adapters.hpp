#pragma once
#include <any>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>
#include <string>
#include <fstream>
#include <format>
#include <concepts>
#include "./db_config.hpp"
#include "./datatypes.hpp"
#include "./models.hpp"

enum OP{
  EQ=1,
  GT,
  LT,
  GTE,
  LTE,
  LIKE,
  ILIKE,
  STARTSWITH,
  ENDSWITH,
  CONTAINS
};

template<typename T, typename... Args>
concept all_same_as_t = (std::convertible_to<T, Args> && ...);

namespace Utils{
using Value_T = std::variant<int, double, std::string>;
inline std::string to_sql_literal(Value_T& value){
  return std::visit([](auto& v)-> std::string{
    if constexpr(std::is_same_v<std::decay_t<decltype(v)>, std::string>) return "'" + v + "'";
    else return std::to_string(v);
  }, value);
}

inline std::string op_to_str(OP op, Value_T v){
  std::string str, value;
  switch (op) {
    case EQ:
      str = " = " + to_sql_literal(v);
      break;
    case GT:
      str = " > " + to_sql_literal(v);
      break;
    case LT:
      str = " < " + to_sql_literal(v);
      break;
    case GTE:
      str = " >= " + to_sql_literal(v);
      break;
    case LTE:
      str = " <= " + to_sql_literal(v);
      break;
    case LIKE:
      str = " like " + to_sql_literal(v);
      break;
    case ILIKE:
      str = " ilike " + to_sql_literal(v);
      break;
    case STARTSWITH:
      value = to_sql_literal(v);
      value.replace(value.end()-1, value.end(), "%'");
      str = " like " + value;
      break;
    case ENDSWITH:
      value = to_sql_literal(v);
      value.replace(value.begin(), value.begin() + 1, "'%");
      str = " like " + value;
      break;
    case CONTAINS:
      value = to_sql_literal(v);
      value.replace(value.end()-1, value.end(), "%'");
      value.replace(value.begin(), value.begin() + 1, "'%");
      str = " like " + value;
      break;
    default:
      throw std::runtime_error("[ERROR: 'filter().build_filter_args().op_to_str()'] => Unknown operator!");
  }
  return str;
}

inline std::any filter_val(Value_T& val){
  return std::visit([](auto& v)->std::any{
    return std::any{v};
  }, val);
}

struct Condition {
  std::string column;
  OP op;
  Value_T value;

  Condition(std::string col, OP op, Value_T v)
  :column(col), op(op), value(v) {}
};
using filters = std::vector<Condition>;

inline std::string build_filter_args(std::string logical_op, filters& filters){
  int op_size = logical_op.size();
  std::string where_str {};
  for(Condition& filter: filters){
    where_str += filter.column + op_to_str(filter.op, filter.value) + " " + logical_op + " ";
  }
  where_str.resize(where_str.size() - (op_size + 2));

  return where_str;
}

typedef struct{
  std::string db_name;
  std::string user;
  std::string passwd;
  std::string host;
  int port;
} db_params;
db_params parse_db_conn_params();

template <typename T, std::size_t N>
struct CustomArray{
  std::array<T, N> wrapped_array {};
  std::size_t index = 0;

  constexpr CustomArray() = default;

  template<all_same_as_t<T>... Args>
  requires (sizeof...(Args) <= N)
  constexpr CustomArray(Args&&... args): wrapped_array {static_cast<T>(args)...}, index(sizeof...(args)){}

  constexpr void push_back(T value){
    if(index >= N) throw "Custom Array is up to capacity!";
    wrapped_array[index++] = value;
  }

  constexpr auto begin(){ return wrapped_array.begin(); }
  constexpr auto begin() const { return wrapped_array.begin(); }
  constexpr auto end(){ return wrapped_array.begin() + index; }
  constexpr auto end() const { return wrapped_array.begin() + index; }

  constexpr T& operator[](std::size_t i){
    if (i >= index) throw "CustomArray: Index out of bounds!";
    return wrapped_array[i];
  }
  constexpr const T& operator[](std::size_t i) const {
    if (i >= index) throw "CustomArray: Index out of bounds!";
    return wrapped_array[i];
  }

  constexpr std::size_t size() const { return index; }
  constexpr std::size_t max_size() const { return N; }
};

template <typename Arg>
std::string to_str(const Arg& arg){
  std::ostringstream ss;
  ss<<arg;
  return ss.str();
}
}

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
}

inline void create_fk_constraint(const std::string& model_name, const std::string& fk_sql_segment,
                                 const std::string& column_name, std::ofstream& Migrations){
  Migrations<< "CONSTRAINT fk_" + column_name + " " + fk_sql_segment;
}

inline void create_uq_constraint(const std::string& uq_col, std::ofstream& Migrations){
  Migrations<<"CONSTRAINT uq_" + uq_col + " UNIQUE (" + uq_col + "),\n";
}

inline void create_column(const std::string& column_name, const std::string& column_sql_attributes, std::ofstream& Migrations){
  Migrations<<column_name + " " + column_sql_attributes;
}

template <typename Col_Map>
void create_table(const std::string& model_name, Col_Map& field_map, std::ofstream& Migrations){
  std::vector<std::string> primary_key_cols;
  std::vector<std::string> unique_constraint_cols;

  Migrations<< "CREATE TABLE IF NOT EXISTS " + model_name + " (\n  " + model_name + "_id SERIAL NOT NULL,\n  ";

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

inline pqxx::connection connect(){
  Utils::db_params params = Utils::parse_db_conn_params();
  try{
    pqxx::connection cxn("dbname=" + params.db_name+
                         " user=" + params.user +
                         " password=" + params.passwd +
                         " host=" + params.host +
                         " port=" + std::to_string(params.port)
                         );
    return cxn;
  }catch (const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'connect()'] => {}", e.what()));
  }
}

template<typename Model_T>
void dbfetch(Model_T& obj, std::string& sql_string, bool getfn_called = false){
  pqxx::connection cxn= connect();
  try{
    pqxx::work txn(cxn);

    if(getfn_called){
      pqxx::result result = txn.exec(sql_string).expect_rows(1);
      obj.records.push_back(result[0]);
      return;
    }

    for(const pqxx::row& row : txn.exec(sql_string)) obj.records.push_back(row);

    txn.commit();
  }catch (const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: in 'db_fetch()'] => {}", e.what()));
  }
  return;
}

template<typename Model_T>
pqxx::connection prepare_insert(){
  Model_T obj {};
  pqxx::placeholders row_vals;
  pqxx::connection cxn = connect();
  std::string insert_statement = "insert into "+ obj.table_name + " (" + obj.col_str +") values(";

  for(int i=0; i<obj.col_map_size; ++i){
    insert_statement += row_vals.get() + ",";
    row_vals.next();
  }

  insert_statement.pop_back();
  insert_statement.append(");");
  cxn.prepare("insert_stmt", insert_statement);

  return cxn;
}

void exec_insert(pqxx::connection& cxn, pqxx::params& p);

namespace query{

template <typename Model_T>
void fetch_all(Model_T& obj, std::string columns){
  std::string sql_string {"select " + columns + " from " + obj.table_name + ";"};
  dbfetch(obj, sql_string);
}

template <typename Model_T, typename... Args>
void get(Model_T& obj, Args... args){
  static_assert(sizeof...(args) > 0 || sizeof...(args)%2 == 0, "[ERROR:'db_adapter::query::get()'] => Args are provided in key-value pairs.");

  std::string sql_kwargs {};
  constexpr int N = sizeof...(args);
  Utils::CustomArray<std::pair<std::string, std::string>, N/2> kwargs {};
  Utils::CustomArray<std::string, N> parsed_args {to_str(args)...};

  for(int i = 0; i < N; i+=2){
    sql_kwargs += parsed_args[i] + "=" + parsed_args[i+1] + " and ";
    kwargs.push_back(std::make_pair(parsed_args[i], parsed_args[i+1]));
  }

  sql_kwargs.replace(sql_kwargs.size()-5, 5, ";");

  if(obj.records.empty()){
    std::string sql_str = "select * from " + obj.table_name + " where " + sql_kwargs;
    dbfetch(obj, sql_str, true);
  }else{
    std::vector<pqxx::row> filtered_rows {};
    for(const pqxx::row& row : obj.records){
      bool accept_row = true;
      for(const auto& kwarg : kwargs){
        if(row.at(kwarg.first).template as<std::string>() != kwarg.second){
          accept_row = false;
          break;
        }
      }
      if(accept_row){
        filtered_rows.push_back(row);
        continue;
      }
      if(filtered_rows.size() == 1) break;
    }
    if(filtered_rows.size() <= obj.records.size()) obj.records = filtered_rows;
    else throw std::runtime_error("[ERROR: get()] => filtered rows are more than the actual initial rows!");
  }
}

inline bool matches_conditions(pqxx::field&& field, OP op, Utils::Value_T v){
  bool accept = false;
  std::any value = Utils::filter_val(v);
  int int_cast = 0;
  double double_cast = 0;
  std::string str_cast {};

  if(value.type() == typeid(int)) int_cast = std::any_cast<int>(value);
  else if(value.type() == typeid(double)) double_cast = std::any_cast<double>(value);
  else if(value.type() == typeid(std::string)) str_cast = std::any_cast<std::string>(value);
  else throw std::invalid_argument("[ERROR: 'filter().match_conditions()'] => Unsupported type passed to filters");

  try{
    switch (op) {
      case EQ:
        if(int_cast) accept = (field.as<int>() == int_cast);
        else if(double_cast) accept = (field.as<double>() == double_cast);
        else if(!str_cast.empty()) accept = (field.as<std::string>().find(str_cast) != std::string::npos);
        else throw std::runtime_error("[ERROR: 'filter().match_conditions()']=> Unsupported type for OP::EQ");
        break;
      case GT:
        if(int_cast) accept = (field.as<int>() > int_cast);
        else if(double_cast) accept = (field.as<double>() > double_cast);
        else throw std::runtime_error("[ERROR: 'filter().match_conditions()']=> Unsupported type for OP::GT)");
        break;
      case GTE:
        if(int_cast) accept = (field.as<int>() >= int_cast);
        else if(double_cast) accept = (field.as<double>() >= double_cast);
        else throw std::runtime_error("[ERROR: 'filter().match_conditions()']=> Unsupported type for OP::GTE)");
        break;
      case LT:
        if(int_cast) accept = (field.as<int>() < int_cast);
        else if(double_cast) accept = (field.as<double>() < double_cast);
        else throw std::runtime_error("[ERROR: 'filter().match_conditions()']=> Unsupported type for OPERAND(OP::LT or OP::LTE)");
        break;
      case LTE:
        if(int_cast) accept = (field.as<int>() <= int_cast);
        else if(double_cast) accept = (field.as<double>() <= double_cast);
        else throw std::runtime_error("[ERROR: 'filter().match_conditions()']=> Unsupported type for OPERAND(OP::LT or OP::LTE)");
        break;
      case LIKE:
      case ILIKE:
        throw std::runtime_error("LIKE/ILIKE not implemented yet for filtering from obj.records.");
        break;
      case STARTSWITH:
      case CONTAINS:
        if(!str_cast.empty()) accept = (field.as<std::string>().find(str_cast) != std::string::npos);
        else throw std::runtime_error("[ERROR: 'filter().match_conditions()']=> Unsupported type for OPERAND(OP::STARTSWITH or OP::CONTAINS)");
        break;
      case ENDSWITH:
        if(!str_cast.empty()){
          std::string field_str = field.as<std::string>();
          accept = (field.as<std::string>().find(str_cast, field_str.size() - str_cast.size()) != std::string::npos);
        } else throw std::runtime_error("[ERROR: 'filter().match_conditions()']=> Unsupported type for OP::ENDSWITH");
        break;
      default:
        throw std::runtime_error("[ERROR: 'filter().match_conditions()'] => Unknown operator!");
    }
  }catch(const std::exception& e){
    throw std::runtime_error(std::format("[ERROR: 'filter().match_conditions()'] => {}", e.what()));
  }
  return accept;
}

template <typename Model_T>
void filter(Model_T& obj, std::string logical_op, Utils::filters& filters){
  if(obj.records.empty()){
    std::string sql_str = "select * from " + obj.table_name + " where " + build_filter_args(logical_op, filters) + ";";
    std::cout<<sql_str<<std::endl;
    dbfetch(obj, sql_str);
  }else{
    std::vector<pqxx::row> filtered_rows {};
    if (logical_op == "and"){
      for(const pqxx::row& row : obj.records){
        bool accept_row = true;
        for(Utils::Condition& filter: filters){
          if(!matches_conditions(row.at(filter.column), filter.op, filter.value)){
            accept_row = false;
            break;
          }
        }
        if(accept_row){
          filtered_rows.push_back(row);
          continue;
        }
      }
    }else if(logical_op == "or"){
      for(const pqxx::row& row : obj.records){
        bool accept_row = false;
        for(Utils::Condition& filter: filters){
          if(matches_conditions(row.at(filter.column), filter.op, filter.value)){
            accept_row = true;
            break;
          }
        }
        if(accept_row){
          filtered_rows.push_back(row);
          continue;
        }
      }
    }else {
      throw std::runtime_error("[ERROR: 'filter()'] => Unknown logical operator for filter fn");
    }

    if(filtered_rows.size() <= obj.records.size()) obj.records = filtered_rows;
    else throw std::runtime_error("[ERROR: 'filter()'] => Filtered rows are more than the actual initial rows!");
  }
}

class JoinBuilder{
  std::string query_str, table_name;
  bool join_pending = true;
public:
  template<typename T>
  JoinBuilder(T& model): table_name(model.table_name) {}

  template<all_same_as_t<std::string>... Args>
  JoinBuilder& select(Args&&... columns){
    query_str = "select " + ((to_str(columns) + ",") + ...);
    query_str.pop_back();
    query_str += " from " + table_name;
    return *this;
  }

  JoinBuilder& inner_join(std::string join_table){
    query_str += " inner join " + join_table;
    if(!join_pending)
      throw std::runtime_error("[ERROR: 'JoinBuilder.inner_join()'] => You have not implemented on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& outer_join(std::string join_table){
    query_str += " outer join " + join_table;
    if(!join_pending)
      throw std::runtime_error("[ERROR: 'JoinBuilder.inner_join()'] => You have not implemented on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& full_join(std::string join_table){
    query_str += " full join " + join_table;
    if(!join_pending)
      throw std::runtime_error("[ERROR: 'JoinBuilder.inner_join()'] => You have not implemented on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& left_join(std::string join_table){
    query_str += " left join " + join_table;
    if(!join_pending)
      throw std::runtime_error("[ERROR: 'JoinBuilder.inner_join()'] => You have not implemented on() yet for the previous join!");
    join_pending = false;
    return *this;
  }
  JoinBuilder& right_join(std::string join_table){
    query_str += " right join " + join_table;
    if(!join_pending)
      throw std::runtime_error("[ERROR: 'JoinBuilder.inner_join()'] => You have not implemented on() yet for the previous join!");
    join_pending = false;
    return *this;
  }

  template<all_same_as_t<std::string>... Args>
  JoinBuilder& on(std::string logical_op, Args&&... conditions){
    if(join_pending)
      throw std::runtime_error("[ERROR: 'JoinBuilder.on()'] => Join pending");
    if(logical_op != "and" && logical_op != "or")
      throw std::runtime_error(std::format("[ERROR: 'JoinBuilder.on()] => Unknown logical operator: {}", logical_op));
    query_str += " on " + ((to_str(conditions) + " " + logical_op + " ") + ...);
    query_str.resize(query_str.size() - (logical_op.size() + 2));
    join_pending = true;
    return *this;
  }

  pqxx::result execute(){
    pqxx::connection cxn = connect();
    try{
      pqxx::work txn {cxn};
      pqxx::result join_results = txn.exec(query_str + ";");
      txn.commit();
      return join_results;
    }catch(const std::exception& e){
      throw std::runtime_error(std::format("[ERROR: 'JoinBuilder.execute()'] => {}", e.what()));
    }
  }

  std::string str(){
    return query_str + ";";
  }
};

template <typename Model_T>
std::vector<Model_T> to_instances(Model_T& obj){
  using tuple_T = decltype(obj.get_attr());
  std::vector<Model_T> instances {};
  instances.reserve(obj.records.size());

  for(const pqxx::row& row : obj.records){
    instances.push_back(Model_T(row.template as_tuple<tuple_T>()));
  }
  return instances;
}

template <typename Model_T>
std::vector<decltype(std::declval<Model_T>().get_attr())> to_values(Model_T& obj){
  using tuple_T = decltype(obj.get_attr());
  std::vector<tuple_T> values {};
  values.reserve(obj.records.size());

  for(const pqxx::row& row : obj.records){
    values.push_back(row.template as_tuple<tuple_T>());
  }

  return values;
}
}

std::optional<pqxx::result> execute_sql(std::string& sql_file_or_str, bool is_file_name = true);

}
namespace db_adapter = psql;
#else
#error "No valid db_engine specified"
#endif
