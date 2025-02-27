#pragma once
#include <string>

std::string to_upper(std::string& str);

namespace psql{

  template <typename IntType>
  void generate_int_sql(IntType& int_obj);

  template <typename CharType>
  void generate_char_sql(CharType& char_obj);

  template <typename DecimalType>
  void generate_decimal_sql(DecimalType& dec_obj);

  template <typename BoolType>
  void generate_bool_sql(BoolType& bool_obj);

  template <typename BinType>
  void generate_bin_sql(BinType& bin_obj);

  template <typename DTType>
  void generate_datetime_sql(DTType& dt_obj);

  template <typename FKType>
  void generate_foreignkey_sql(FKType& fk_obj);

}
