#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include "json.hpp"

class FieldAttr{
public:
	std::string ctype, datatype, sql_segment;
	bool primary_key, not_null, unique;

	FieldAttr(std::string ct = "null",std::string dt = "null", bool nn = false, bool uq = false, bool pk = false)
	: ctype(ct), datatype(dt), not_null(nn), unique(uq), primary_key(pk)
	{}

  ~FieldAttr() = default;
};

class IntegerField: public FieldAttr{
public:
	std::string check_condition;
	int check_constraint;

  IntegerField() = default;
	IntegerField(std::string datatype, bool pk = false, bool not_null = false, bool unique = false,
              int check_constr = 0, std::string check_cond = "");

  ~IntegerField() = default;
};
void to_json(nlohmann::json& j, const IntegerField& field);
void from_json(const nlohmann::json& j, IntegerField& field);

class DecimalField: public FieldAttr{
public:
	int max_length, decimal_places;

  DecimalField() = default;
	DecimalField(std::string datatype, int max_length, int decimal_places, bool pk = false);

  ~DecimalField() = default;
};
void to_json(nlohmann::json& j, const DecimalField& field);
void from_json(const nlohmann::json& j, DecimalField& field);

class CharField: public FieldAttr{
public:
	int length;

  CharField() = default;
	CharField(std::string datatype, int length = 0, bool not_null = false, bool unique = false, bool pk = false);

  ~CharField() = default;
};
void to_json(nlohmann::json& j, const CharField& field);
void from_json(const nlohmann::json& j, CharField& field);

class BoolField:public FieldAttr{
public:
	bool enable_default, default_value;

	BoolField(bool not_null = false, bool enable_default = false, bool default_value = false);

  ~BoolField() = default;
};
void to_json(nlohmann::json& j, const BoolField& field);
void from_json(const nlohmann::json& j, BoolField& field);

class BinaryField: public FieldAttr{
public:
  BinaryField() = default;
	BinaryField(bool not_null, bool unique = false, bool pk = false);

  ~BinaryField() = default;
};
void to_json(nlohmann::json& j, const BinaryField& field);
void from_json(const nlohmann::json& j, BinaryField& field);

class DateTimeField:public FieldAttr{
public:
	bool enable_default;
	std::string default_val;

  DateTimeField() = default;
	DateTimeField(std::string datatype, bool enable_default = false, std::string default_val = "", bool pk = false);

  ~DateTimeField() = default;
};
void to_json(nlohmann::json& j, const DateTimeField& field);
void from_json(const nlohmann::json& j, DateTimeField& field);

class ForeignKey : public FieldAttr{
public:
  std::string col_name, sql_type, model_name, ref_col_name, on_delete, on_update;

  ForeignKey() = default;
	ForeignKey(std::string cn, std::string mn, std::string rcn, std::optional<FieldAttr> pk_col_obj=std::nullopt,
             std::string on_del="CASCADE", std::string on_upd="CASCADE");

  ~ForeignKey() = default;
};
void to_json(nlohmann::json& j, const ForeignKey& field);
void from_json(const nlohmann::json& j, ForeignKey& field);

using DataTypeVariant = std::variant<std::shared_ptr<IntegerField>,std::shared_ptr<CharField>, std::shared_ptr<BoolField>,
                                     std::shared_ptr<BinaryField>, std::shared_ptr<DateTimeField>, std::shared_ptr<ForeignKey>,
                                     std::shared_ptr<DecimalField>
                                    >;

void variant_to_json(nlohmann::json& j, const DataTypeVariant& variant);
void variant_from_json(const nlohmann::json& j, DataTypeVariant& variant);
