#pragma once
#include <string>
#include <variant>
#include "json.hpp"
#include "./db_engine_adapters.hpp"

namespace db_adapter = psql;

std::string to_upper(std::string& str);

class FieldAttr{
public:
	std::string ctype, datatype, sql_segment;
	bool primary_key, not_null, unique;

	FieldAttr(std::string ct = "null",std::string dt = "null", bool nn = false, bool uq = false, bool pk = false)
	: ctype(ct), datatype(dt), not_null(nn), unique(uq), primary_key(pk)
	{}
};

class IntegerField: public FieldAttr{
public:
	std::string check_condition;
	int check_constraint;

  IntegerField() = default;
	IntegerField(std::string datatype, bool pk = false, bool not_null = false,
               bool unique = false, int check_constr = 0, std::string check_cond = "default")
    :FieldAttr("int", datatype, not_null, unique, pk), check_constraint(check_constr), check_condition(check_cond)
 	{
    db_adapter::generate_int_sql(*this);
	}
};
void to_json(nlohmann::json& j, const IntegerField& field);
void from_json(const nlohmann::json& j, IntegerField& field);

class DecimalField: public FieldAttr{
public:
	int max_length, decimal_places;

  DecimalField() = default;
	DecimalField(std::string datatype, int max_length, int decimal_places, bool pk = false)
    :FieldAttr("float", datatype, false, false, pk), max_length(max_length), decimal_places(decimal_places)
	{
    db_adapter::generate_decimal_sql(*this);
	}
};
void to_json(nlohmann::json& j, const DecimalField& field);
void from_json(const nlohmann::json& j, DecimalField& field);

class CharField: public FieldAttr{
public:
	int length;

  CharField() = default;
	CharField(std::string datatype, int length = 0, bool not_null = false, bool unique = false, bool pk = false)
    :FieldAttr("std::string", datatype, not_null, unique, pk), length(length)
	{
    db_adapter::generate_char_sql(*this);
	}
};
void to_json(nlohmann::json& j, const CharField& field);
void from_json(const nlohmann::json& j, CharField& field);

class BoolField:public FieldAttr{
public:
	bool enable_default, default_value;

	BoolField(bool not_null = false, bool enable_default = false, bool default_value = false)
	:FieldAttr("bool", "BOOLEAN", not_null, false, false), enable_default(enable_default), default_value(default_value)
	{
    db_adapter::generate_bool_sql(*this);
	}
};
void to_json(nlohmann::json& j, const BoolField& field);
void from_json(const nlohmann::json& j, BoolField& field);

class BinaryField: public FieldAttr{
public:
	int size;

  BinaryField() = default;
	BinaryField(int size, bool not_null = false, bool unique = false, bool pk = false)
  :FieldAttr("int", "BYTEA", not_null, unique, pk),size(size)
	{
    db_adapter::generate_bin_sql(*this);
	}
};
void to_json(nlohmann::json& j, const BinaryField& field);
void from_json(const nlohmann::json& j, BinaryField& field);

class DateTimeField:public FieldAttr{
public:
	bool enable_default;
	std::string default_val;

  DateTimeField() = default;
	DateTimeField(std::string datatype, bool enable_default = false, std::string default_val = "default", bool pk = false)
  :FieldAttr("std::string",datatype, false, false, pk), enable_default(enable_default), default_val(default_val)
	{
    db_adapter::generate_datetime_sql(*this);
	}
};
void to_json(nlohmann::json& j, const DateTimeField& field);
void from_json(const nlohmann::json& j, DateTimeField& field);

class ForeignKey : public FieldAttr{
public:
  std::string col_name, model_name, ref_col_name, on_delete, on_update;

	ForeignKey(std::string cn,std::string mn="def", std::string rcn="def", std::string on_del="def", std::string on_upd="def")
	:FieldAttr("null", "FOREIGN KEY", false, false, false),
  col_name(cn), model_name(mn), ref_col_name(cn), on_delete(on_del), on_update(on_upd)
	{
    db_adapter::generate_foreignkey_sql(*this);
  }
};
void to_json(nlohmann::json& j, const ForeignKey& field);
void from_json(const nlohmann::json& j, ForeignKey& field);

using DataTypeVariant = std::variant<IntegerField, CharField, BoolField, BinaryField, DateTimeField, ForeignKey, DecimalField>;

void variant_to_json(nlohmann::json& j, const DataTypeVariant& variant);
void variant_from_json(const nlohmann::json& j, DataTypeVariant& variant);
