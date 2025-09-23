#include "../../include/strata/psql/sql_generators.hpp"
#include "../../include/strata/psql/utils.hpp"
#include <format>

namespace psql {
void generate_int_sql(IntegerField& int_obj){
  int_obj.datatype = Utils::str_to_upper(int_obj.datatype);
  if(int_obj.datatype != "INTEGER" && int_obj.datatype != "SMALLINT" && int_obj.datatype != "BIGINT"){
    throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL. Provide a valid datatype", int_obj.datatype));
  }
  int_obj.sql_segment += int_obj.datatype;
  if (int_obj.not_null) int_obj.sql_segment = int_obj.datatype + " NOT NULL";
}

void generate_char_sql(CharField& char_obj){
  char_obj.datatype = Utils::str_to_upper(char_obj.datatype);
  if(char_obj.datatype != "VARCHAR" && char_obj.datatype != "CHAR" && char_obj.datatype != "TEXT"){
    throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL. Provide a valid datatype", char_obj.datatype));
  }
  char_obj.sql_segment += char_obj.datatype;
  if (char_obj.length == 0 && char_obj.datatype != "TEXT"){
    throw std::runtime_error(std::format("Length attribute is required for datatype '{}'", char_obj.datatype));
  }
  if (char_obj.datatype != "TEXT") {
    char_obj.sql_segment += "(" + std::to_string(char_obj.length) + ")";
  }
  if (char_obj.not_null) char_obj.sql_segment +=  " NOT NULL";
}

void generate_decimal_sql(DecimalField& dec_obj){
  dec_obj.datatype = Utils::str_to_upper(dec_obj.datatype);

  if(dec_obj.datatype != "DECIMAL" && dec_obj.datatype != "REAL" &&
    dec_obj.datatype != "DOUBLE PRECISION" && dec_obj.datatype != "NUMERIC"){
    throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL. Provide a valid datatype", dec_obj.datatype));
  }

  if(dec_obj.datatype == "REAL" || dec_obj.datatype == "DOUBLE PRECISION"){
    dec_obj.sql_segment = dec_obj.datatype;
    return;
  }

  if(dec_obj.max_length > 0 || dec_obj.decimal_places > 0)
    dec_obj.sql_segment = dec_obj.datatype + "(" + std::to_string(dec_obj.max_length) + "," + std::to_string(dec_obj.decimal_places) + ")";
  else
    throw std::runtime_error(std::format("Max length and/or decimal places cannot be 0 for datatype '{}'", dec_obj.datatype));
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
  bin_obj.sql_segment = bin_obj.datatype;
  if(bin_obj.not_null) bin_obj.sql_segment += " NOT NULL";
}

void generate_datetime_sql(DateTimeField& dt_obj){
  dt_obj.datatype = Utils::str_to_upper(dt_obj.datatype);
  if(dt_obj.datatype != "DATE" && dt_obj.datatype != "TIME" && dt_obj.datatype != "TIMESTAMP_WTZ" &&
    dt_obj.datatype != "TIMESTAMP" && dt_obj.datatype != "TIME_WTZ"  && dt_obj.datatype != "INTERVAL"){
    throw std::runtime_error(std::format("Datatype '{}' not supported in postgreSQL. Provide a valid datatype", dt_obj.datatype));
    return;
  }

  std::string::size_type n = dt_obj.datatype.find('_');
  if(n != std::string::npos){
    dt_obj.datatype.replace(n+1, n+3, "WITH TIME ZONE");
  }

  dt_obj.sql_segment = dt_obj.datatype;
  if(dt_obj.enable_default && !dt_obj.default_val.empty()){
    dt_obj.default_val = Utils::str_to_upper(dt_obj.default_val);
    dt_obj.sql_segment += " DEFAULT " + dt_obj.default_val;
  }
}

void generate_foreignkey_sql(ForeignKey& fk_obj){
  fk_obj.sql_segment ="FOREIGN KEY(" + fk_obj.col_name + ") REFERENCES " + fk_obj.model_name + " (" + fk_obj.ref_col_name + ")";
  fk_obj.on_delete = Utils::str_to_upper(fk_obj.on_delete);
  fk_obj.sql_segment += " ON DELETE " + fk_obj.on_delete;
  fk_obj.on_update = Utils::str_to_upper(fk_obj.on_update);
  fk_obj.sql_segment += " ON UPDATE " + fk_obj.on_update;
}

}
