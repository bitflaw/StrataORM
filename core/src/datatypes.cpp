#include "../includes/datatypes.hpp"
#include <variant>
#include "../includes/db_engine_adapters.hpp"

template <typename T>
bool try_set_variant(const nlohmann::json& j, DataTypeVariant& variant) {
  try {
    T value;
    from_json(j, value);
    variant = std::move(value);
    return true;
  }catch (const std::exception& e) {
    return false;
  }
}

void to_json(nlohmann::json& j, const IntegerField& field){
  j = nlohmann::json{
    {"datatype", field.datatype},
    {"not_null", field.not_null},
    {"unique", field.unique},
    {"primary_key", field.primary_key},
    {"check_constraint", field.check_constraint},
    {"check_condition", field.check_condition}
  };
}

void from_json(const nlohmann::json& j, IntegerField& field){
  field.datatype = j.at("datatype").get<std::string>();
  field.not_null = j.at("not_null").get<bool>();
  field.unique = j.at("unique").get<bool>();
  field.primary_key = j.at("primary_key").get<bool>();
  field.check_constraint = j.at("check_constraint").get<int>();
  field.check_condition = j.at("check_condition").get<std::string>();
  db_adapter::generate_int_sql(field);
}

void to_json(nlohmann::json& j, const DecimalField& field){
  j = nlohmann::json{
    {"datatype", field.datatype},
    {"primary_key", field.primary_key},
    {"max_length", field.max_length},
    {"dec_places", field.decimal_places}
  };
}

void from_json(const nlohmann::json& j, DecimalField& field){
  field.datatype = j.at("datatype").get<std::string>();
  field.primary_key = j.at("primary_key").get<bool>();
  field.max_length = j.at("max_length").get<int>();
  field.decimal_places = j.at("dec_places").get<int>();
  db_adapter::generate_decimal_sql(field);
}

void to_json(nlohmann::json& j, const CharField& field){
  j = nlohmann::json{
    {"datatype", field.datatype},
    {"not_null", field.not_null},
    {"unique", field.unique},
    {"primary_key", field.primary_key},
    {"length", field.length}
  };
}

void from_json(const nlohmann::json& j, CharField& field){
  field.datatype = j.at("datatype").get<std::string>();
  field.not_null = j.at("not_null").get<bool>();
  field.unique = j.at("unique").get<bool>();
  field.primary_key = j.at("primary_key").get<bool>();
  field.length = j.at("length").get<int>();
  db_adapter::generate_char_sql(field);
}

void to_json(nlohmann::json& j, const BoolField& field){
  j = nlohmann::json{
    {"not_null", field.not_null},
    {"enable_def", field.enable_default},
    {"default", field.default_value}
  };
}

void from_json(const nlohmann::json& j, BoolField& field){
  field.datatype = "BOOLEAN";
  field.not_null = j.at("not_null").get<bool>();
  field.enable_default = j.at("enable_def").get<bool>();
  field.default_value = j.at("default").get<bool>();
  db_adapter::generate_bool_sql(field);
}

void to_json(nlohmann::json& j, const BinaryField& field){
  j = nlohmann::json{
    {"not_null", field.not_null},
    {"unique", field.unique},
    {"primary_key", field.primary_key},
    {"size", field.size}
  };
}

void from_json(const nlohmann::json& j, BinaryField& field){
  field.datatype = "BYTEA";
  field.not_null = j.at("not_null").get<bool>();
  field.unique = j.at("unique").get<bool>();
  field.primary_key = j.at("primary_key").get<bool>();
  field.size = j.at("size").get<int>();
  db_adapter::generate_bin_sql(field);
}

void to_json(nlohmann::json& j, const DateTimeField& field){
  j = nlohmann::json{
    {"datatype", field.datatype},
    {"primary_key", field.primary_key},
    {"default_value", field.default_val},
    {"enable_def", field.enable_default}
  };
}

void from_json(const nlohmann::json& j, DateTimeField& field){
  field.datatype = j.at("datatype").get<std::string>();
  field.primary_key = j.at("primary_key").get<bool>();
  field.enable_default = j.at("enable_def").get<bool>();
  field.default_val = j.at("default_value").get<std::string>();
  db_adapter::generate_datetime_sql(field);
}

void to_json(nlohmann::json& j, const ForeignKey& field){
  j = nlohmann::json{
    {"column_name", field.col_name},
    {"model_name", field.model_name},
    {"referenced_column_name", field.ref_col_name},
    {"on_delete", field.on_delete},
    {"on_update", field.on_update},
  };
}

void from_json(const nlohmann::json& j, ForeignKey& field){
  field.datatype = "FOREIGN KEY";
  field.col_name = j.at("column_name").get<std::string>();
  field.model_name = j.at("model_name").get<std::string>();
  field.ref_col_name = j.at("referenced_column_name").get<std::string>();
  field.on_delete = j.at("on_delete").get<std::string>();
  field.on_update = j.at("on_update").get<std::string>(); 
  db_adapter::generate_foreignkey_sql(field);
}

void variant_to_json(nlohmann::json& j, const DataTypeVariant& variant){
  std::visit([&j](auto& arg) mutable {
    to_json(j, arg);
  }, variant);
}

template <typename... Ts>
bool try_deserialize(const nlohmann::json& j, DataTypeVariant& variant, std::variant<Ts...>*) {
  return ((try_set_variant<Ts>(j, variant)) || ...);
}

void variant_from_json(const nlohmann::json& j, DataTypeVariant& variant) {
  if (!try_deserialize(j, variant, static_cast<DataTypeVariant*>(nullptr))) {
    throw std::invalid_argument("Error occured while parsing JSON back to objects.");
  }
}
