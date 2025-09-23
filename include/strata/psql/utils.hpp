#pragma once
#include <string>
#include <vector>
#include <any>
#include <variant>
#include <sstream>

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

namespace Utils{

std::string str_to_upper(std::string& str);

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

using dbenvars = std::vector<std::pair<std::string, std::string>>;
void set_dbenvars(dbenvars&);

typedef struct{
  std::string db_name;
  std::string user;
  std::string passwd;
  std::string host;
  int port;
} db_params;
db_params parse_dbenvars();

template <typename T>
std::string to_str(T& arg){
  std::ostringstream ss;
  ss<<arg;
  return ss.str();
}

} //Utils namespace

