#include <stdexcept>
#include <variant>
#include "../orm++/db_adapters.hpp"

IntegerField::IntegerField(std::string datatype, bool pk, bool not_null, bool unique, int check_constr, std::string check_cond)
:FieldAttr("int", datatype, not_null, unique, pk), check_constraint(check_constr), check_condition(check_cond)
{
  db_adapter::generate_int_sql(*this);
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

DecimalField::DecimalField(std::string datatype, int max_length, int decimal_places, bool pk)
  :FieldAttr("float", datatype, false, false, pk), max_length(max_length), decimal_places(decimal_places)
{
  db_adapter::generate_decimal_sql(*this);
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

CharField::CharField(std::string datatype, int length, bool not_null, bool unique, bool pk)
  :FieldAttr("std::string", datatype, not_null, unique, pk), length(length)
{
  db_adapter::generate_char_sql(*this);
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

BoolField::BoolField(bool not_null, bool enable_default, bool default_value)
:FieldAttr("bool", "BOOLEAN", not_null, false, false), enable_default(enable_default), default_value(default_value)
{
  db_adapter::generate_bool_sql(*this);
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

BinaryField::BinaryField(bool not_null, bool unique, bool pk)
:FieldAttr("int", "BYTEA", not_null, unique, pk)
{
  db_adapter::generate_bin_sql(*this);
}

void to_json(nlohmann::json& j, const BinaryField& field){
  j = nlohmann::json{
    {"not_null", field.not_null},
    {"unique", field.unique},
    {"primary_key", field.primary_key},
  };
}

void from_json(const nlohmann::json& j, BinaryField& field){
  field.datatype = "BYTEA";
  field.not_null = j.at("not_null").get<bool>();
  field.unique = j.at("unique").get<bool>();
  field.primary_key = j.at("primary_key").get<bool>();
  db_adapter::generate_bin_sql(field);
}

DateTimeField::DateTimeField(std::string datatype, bool enable_default, std::string default_val, bool pk)
:FieldAttr("std::string",datatype, false, false, pk), enable_default(enable_default), default_val(default_val)
{
  db_adapter::generate_datetime_sql(*this);
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
ForeignKey::ForeignKey(std::string cn, std::string mn, std::string rcn, std::optional<FieldAttr> pk_col_obj, std::string on_del, std::string on_upd)
:FieldAttr("null", "FOREIGN KEY", false, false, false),
col_name(cn), model_name(mn), ref_col_name(rcn), on_delete(on_del), on_update(on_upd)
{

  if(pk_col_obj.has_value()){
    ctype = pk_col_obj->ctype;
    sql_type = pk_col_obj->sql_segment;
  }else{
    ctype="int";
    sql_type = "INTEGER NOT NULL";
  }

  db_adapter::generate_foreignkey_sql(*this);
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

template <typename T>
bool try_set_variant(const nlohmann::json& j, DataTypeVariant& variant) {
  try {
    T value_sptr = std::make_shared<typename T::element_type>();
    from_json(j, *value_sptr);
    variant = value_sptr;
    return true;
  }catch (const std::exception& e) {
    return false;
  }
}

template <typename... Ts>
bool try_deserialize(const nlohmann::json& j, DataTypeVariant& variant, std::variant<Ts...>*) {
  return ((try_set_variant<Ts>(j, variant)) || ...);
}

void variant_to_json(nlohmann::json& j, const DataTypeVariant& variant){
  std::visit([&j](auto& arg) mutable {
    to_json(j, *arg);
  }, variant);
}

void variant_from_json(const nlohmann::json& j, DataTypeVariant& variant) {
  if (!try_deserialize(j, variant, static_cast<DataTypeVariant*>(nullptr))) {
    throw std::invalid_argument("Error occured while parsing JSON back to objects.");
  }
}
