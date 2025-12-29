#pragma once
#include "../db_config.hpp"

#ifdef PSQL

#include <stdexcept>
#include <typeinfo>
#include <memory>
#include <variant>
#include <format>
#include "../json.hpp"
#include "../field_base.hpp"
#include "../utils.hpp"
#include "alterers.hpp"
#include "create_constraints.hpp"
#include "deleters.hpp"

namespace psql::Field {

class IntegerField: public FieldAttr{
public:
	std::string check_condition;
	int check_constraint;

  IntegerField() = default;
	IntegerField(std::string datatype, bool pk=false, bool not_null=false, bool unique=false,
               int check_constr=0, std::string check_cond="")
  :FieldAttr("int", datatype, not_null, unique, pk), check_condition(check_cond), check_constraint(check_constr)
  {
    gen_sql();
  }

  void gen_sql() override
  {
    datatype = Utils::str_to_upper(datatype);
    if(datatype != "INTEGER" && datatype != "SMALLINT" && datatype != "BIGINT"){
      throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL.", datatype));
    }
    sql_segment += datatype;
    if (not_null) sql_segment += " NOT NULL";

  }

  void to_json (nlohmann::json& j) const override
  {
    j = nlohmann::json{
      {"datatype", datatype},
      {"not_null", not_null},
      {"unique", unique},
      {"primary_key", primary_key},
      {"check_constraint", check_constraint},
      {"check_condition", check_condition}
    };
  }

  void from_json (const nlohmann::json& j) override
  {
    datatype = j.at("datatype").get<std::string>();
    not_null = j.at("not_null").get<bool>();
    unique = j.at("unique").get<bool>();
    primary_key = j.at("primary_key").get<bool>();
    check_constraint = j.at("check_constraint").get<int>();
    check_condition = j.at("check_condition").get<std::string>();
    gen_sql();
  }

  void track (std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
              std::ofstream& Migrations, const nlohmann::json& frm = {}) const override
  {
    try {
      IntegerField int_obj = dynamic_cast<IntegerField&>(old_col_obj);
      if(int_obj.datatype != datatype)
      {
        db::alter_column_type(new_model_name, col_name, datatype, Migrations);
      }
      //if((int_obj.check_condition != check_condition) && check_condition != ""){
      //    string check = "CHECK(" + col_name + check_condition + std::to_string(check_constraint) + ")";
      //    Migrations << "ALTER TABLE " + new_it->first + " ALTER COLUMN " + alterations + ";\n";
      //}
    } catch (std::bad_cast& e) {
      throw std::runtime_error(e.what());
    }
  }

  ~IntegerField () = default;
};

class DecimalField: public FieldAttr{
public:
	int max_length, decimal_places;

  DecimalField() = default;
  DecimalField(std::string datatype, int max_length, int decimal_places, bool pk=false)
  :FieldAttr("float", datatype, false, false, pk), max_length(max_length), decimal_places(decimal_places)
  {
    gen_sql();
  }

  void gen_sql() override
  {
    datatype = Utils::str_to_upper(datatype);
    if(datatype != "DECIMAL" && datatype != "REAL" &&
      datatype != "DOUBLE PRECISION" && datatype != "NUMERIC")
    {
      throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL. Provide a valid datatype", datatype));
    }
    if(datatype == "REAL" || datatype == "DOUBLE PRECISION")
    {
      sql_segment = datatype;
      return;
    }

    if(max_length > 0 || decimal_places > 0)
      sql_segment = datatype + "(" + std::to_string(max_length) + "," + std::to_string(decimal_places) + ")";
    else
      throw std::runtime_error(std::format("Max length and/or decimal places cannot be 0 for datatype '{}'", datatype));
  }

  void to_json(nlohmann::json& j) const override
  {
    j = nlohmann::json{
      {"datatype", datatype},
      {"primary_key", primary_key},
      {"max_length", max_length},
      {"dec_places", decimal_places}
    };
  }
  void from_json(const nlohmann::json& j) override
  {
    datatype = j.at("datatype").get<std::string>();
    primary_key = j.at("primary_key").get<bool>();
    max_length = j.at("max_length").get<int>();
    decimal_places = j.at("dec_places").get<int>();
    gen_sql();
  }
  void track(std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
             std::ofstream& Migrations, const nlohmann::json& frm = {}) const override
  {
    try {
      DecimalField init_obj = dynamic_cast<DecimalField&>(old_col_obj);
      std::string alterations {};
      if(init_obj.datatype != datatype ||
        init_obj.max_length != max_length ||
        init_obj.decimal_places != decimal_places)
      {
        alterations = datatype + " (" + std::to_string(max_length) + "," + std::to_string(decimal_places) + ")";
        db::alter_column_type(new_model_name, col_name, alterations, Migrations);
      }
    } catch (std::bad_cast& e) {
      throw std::runtime_error(
        std::format("[ERROR: in DecimalField::track()] {}", e.what())
      );
    }
  }

  ~DecimalField() = default;
};

class CharField: public FieldAttr{
public:
	int length;

  CharField() = default;
  CharField(std::string datatype, int length=0, bool not_null=false, bool unique=false, bool pk=false)
  :FieldAttr("std::string", datatype, not_null, unique, pk), length(length)
  {
    gen_sql();
  }

  void gen_sql () override
  {
    datatype = Utils::str_to_upper(datatype);
    if(datatype != "VARCHAR" && datatype != "CHAR" && datatype != "TEXT")
      throw std::runtime_error(std::format("Datatype '{}' is not supported by postgreSQL.", datatype));
    sql_segment += datatype;
    if (length == 0 && datatype != "TEXT")
      throw std::runtime_error(std::format("Length attribute is required for datatype '{}'", datatype));
    if (datatype != "TEXT")
      sql_segment += "(" + std::to_string(length) + ")";
    if (not_null) sql_segment +=  " NOT NULL";
  }

  void to_json(nlohmann::json& j) const override
  {
    j = nlohmann::json{
      {"datatype", datatype},
      {"not_null", not_null},
      {"unique", unique},
      {"primary_key", primary_key},
      {"length", length}
    };
  }

  void from_json(const nlohmann::json& j) override
  {
    datatype = j.at("datatype").get<std::string>();
    not_null = j.at("not_null").get<bool>();
    unique = j.at("unique").get<bool>();
    primary_key = j.at("primary_key").get<bool>();
    length = j.at("length").get<int>();
    gen_sql();
  }

  void track (std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
              std::ofstream& Migrations, const nlohmann::json& frm = {}) const override
  {
    try
    {
      CharField init_obj = dynamic_cast<CharField&>(old_col_obj);
      std::string alterations {};
      if((init_obj.datatype != datatype) || (init_obj.length != length))
      {
        alterations = "VARCHAR( " + std::to_string(length) + " )";
        db::alter_column_type(new_model_name, col_name, alterations, Migrations);
      }
    } catch (std::bad_cast& e)
    {
      throw std::runtime_error(
        std::format("[ERROR: in CharField::track()] {}", e.what())
      );
    }
  }

  ~CharField() = default;
};

class BoolField : public FieldAttr{
public:
	bool enable_default, default_value;

  BoolField(bool not_null=false, bool enable_default=false, bool default_value=false)
  :FieldAttr("bool", "BOOLEAN", not_null, false, false), enable_default(enable_default), default_value(default_value)
  {
    gen_sql();
  }

  void gen_sql() override
  {
    sql_segment += datatype;
    if (not_null) sql_segment += " NOT NULL";
    if (enable_default)
    {
      if(default_value) sql_segment += " DEFAULT TRUE";
      else sql_segment += " DEFAULT FALSE";
    }
  }

  void to_json(nlohmann::json& j) const override
  {
    j = nlohmann::json{
      {"not_null", not_null},
      {"enable_def", enable_default},
      {"default", default_value}
    };
  }

  void from_json(const nlohmann::json& j) override
  {
    datatype = "BOOLEAN";
    not_null = j.at("not_null").get<bool>();
    enable_default = j.at("enable_def").get<bool>();
    default_value = j.at("default").get<bool>();
    gen_sql();
  }

  void track (std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
              std::ofstream& Migrations, const nlohmann::json& frm = {}) const override
  {
    try
    {
      std::string alterations {};
      BoolField init_obj = dynamic_cast<BoolField&>(old_col_obj);
      if(init_obj.enable_default != enable_default){
        if(enable_default){
          db::alter_column_defaultval(new_model_name, col_name, true, std::to_string(default_value), Migrations);
        }else{
          alterations = col_name + " DROP DEFAULT";
          db::alter_column_defaultval(new_model_name, col_name, false, "false", Migrations);
        }
      }
    } catch (std::bad_cast& e)
    {
      throw std::runtime_error(
        std::format("[ERROR: in BoolField::track()] {}", e.what())
      );
    }
  }
  ~BoolField() = default;
};

class BinaryField: public FieldAttr{
public:
  BinaryField() = default;
  BinaryField(bool not_null, bool unique=false, bool pk=false)
  :FieldAttr("int", "BYTEA", not_null, unique, pk)
  {
    gen_sql();
  }

  void gen_sql() override
  {
    sql_segment = datatype;
    if(not_null) sql_segment += " NOT NULL";
  }

  void to_json(nlohmann::json& j) const override
  {
    j = nlohmann::json{
      {"not_null", not_null},
      {"unique", unique},
      {"primary_key", primary_key},
    };
  }

  void from_json(const nlohmann::json& j) override
  {
    datatype = "BYTEA";
    not_null = j.at("not_null").get<bool>();
    unique = j.at("unique").get<bool>();
    primary_key = j.at("primary_key").get<bool>();
    gen_sql();
  }

  void track (std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
              std::ofstream& Migrations, const nlohmann::json& frm = {}) const override
  { return; }

  ~BinaryField() = default;
};

class DateTimeField:public FieldAttr{
public:
	bool enable_default;
	std::string default_val;

  DateTimeField() = default;
  DateTimeField(std::string datatype, bool enable_default=false, std::string default_val="", bool pk=false)
  :FieldAttr("std::string",datatype, false, false, pk), enable_default(enable_default), default_val(default_val)
  {
    gen_sql();
  }

  void gen_sql() override
  {
    datatype = Utils::str_to_upper(datatype);
    if(datatype != "DATE" && datatype != "TIME" && datatype != "TIMESTAMP_WTZ" &&
      datatype != "TIMESTAMP" && datatype != "TIME_WTZ"  && datatype != "INTERVAL"){
      throw std::runtime_error(std::format("Datatype '{}' not supported in postgreSQL.", datatype));
    }

    std::string::size_type n = datatype.find('_');
    if(n != std::string::npos){
      datatype.replace(n+1, n+3, "WITH TIME ZONE");
    }

    sql_segment = datatype;
    if(enable_default && !default_val.empty()){
      default_val = Utils::str_to_upper(default_val);
      sql_segment += " DEFAULT " + default_val;
    }
  }
  void to_json(nlohmann::json& j) const override
  {
    j = nlohmann::json{
      {"datatype", datatype},
      {"primary_key", primary_key},
      {"default_value", default_val},
      {"enable_def", enable_default}
    };
  }

  void from_json(const nlohmann::json& j) override
  {
    datatype = j.at("datatype").get<std::string>();
    primary_key = j.at("primary_key").get<bool>();
    enable_default = j.at("enable_def").get<bool>();
    default_val = j.at("default_value").get<std::string>();
    gen_sql();
  }

  void track (std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
              std::ofstream& Migrations, const nlohmann::json& frm = {}) const override
  {
    try
    {
      DateTimeField init_obj = dynamic_cast<DateTimeField&>(old_col_obj);
      if(init_obj.datatype != datatype){
        db::alter_column_type(new_model_name, col_name, datatype, Migrations);
      }
      if((init_obj.enable_default != enable_default) && enable_default){
        db::alter_column_defaultval(new_model_name, col_name, true, default_val, Migrations);
      }else{
        db::alter_column_defaultval(new_model_name, col_name, false, default_val, Migrations);
      }
    } catch (std::bad_cast& e)
    {
      throw std::runtime_error(
        std::format("[ERROR: in DateTimeField::track()] {}", e.what())
      );
    }
  }

  ~DateTimeField() = default;
};

class ForeignKey : public FieldAttr{
public:
  std::string col_name, sql_type, model_name, ref_col_name, on_delete, on_update;

  ForeignKey() = default;
	ForeignKey(std::string cn, std::string mn, std::string rcn, std::string ctype="int",
            std::string rsql="INTEGER NOT NULL", std::string on_del="CASCADE", std::string on_upd="CASCADE")
    :FieldAttr(ctype, "FOREIGN KEY", false, false, false),
    col_name(cn), model_name(mn), ref_col_name(rcn), on_delete(on_del), on_update(on_upd)
  {
    sql_type = rsql;
    gen_sql();
  }

  void gen_sql() override
  {
    sql_segment ="FOREIGN KEY(" + col_name + ") REFERENCES " + model_name + " (" + ref_col_name + ")";
    on_delete = Utils::str_to_upper(on_delete);
    sql_segment += " ON DELETE " + on_delete;
    on_update = Utils::str_to_upper(on_update);
    sql_segment += " ON UPDATE " + on_update;
  }

  void to_json(nlohmann::json& j) const override
  {
    j = nlohmann::json{
      {"column_name", col_name},
      {"model_name", model_name},
      {"referenced_column_name", ref_col_name},
      {"ctype", ctype},
      {"sql_type", sql_type},
      {"on_delete", on_delete},
      {"on_update", on_update},
    };
  }

  void from_json(const nlohmann::json& j) override
  {
    datatype = "FOREIGN KEY";
    col_name = j.at("column_name").get<std::string>();
    model_name = j.at("model_name").get<std::string>();
    ref_col_name = j.at("referenced_column_name").get<std::string>();
    ctype = j.at("ctype").get<std::string>();
    sql_type = j.at("sql_type").get<std::string>();
    on_delete = j.at("on_delete").get<std::string>();
    on_update = j.at("on_update").get<std::string>();
    gen_sql();
  }

  void track (std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
              std::ofstream& Migrations, const nlohmann::json& frm) const override
  {
    try
    {
      std::string constraint_name = "fk_";
      for(auto& [model_name, col_renames]: frm.items()){
        if(model_name == new_model_name){
          for(auto& [old_cn, new_cn] : col_renames.items()){
            if(col_name == new_cn.get<std::string>()){
              constraint_name = constraint_name + "_" + old_cn;
            }else{
              constraint_name = constraint_name + "_" + col_name;
            }
          }
        }else{
          constraint_name = constraint_name + "_" + col_name;
        }
      }
      db::drop_constraint(new_model_name, constraint_name, Migrations);
      Migrations<<"ALTER TABLE " + new_model_name + " ADD ";
      db::create_fk_constraint(sql_segment, col_name, Migrations);
    } catch (std::bad_cast& e)
    {
      throw std::runtime_error(
        std::format("[ERROR: in ForeignKey::track()] {}", e.what())
      );
    }
  }
  ~ForeignKey() = default;
};

}// INFO: namespace psql::Field
namespace psql {

using DataTypeVariant = std::variant<Field::IntegerField, Field::CharField, Field::BoolField,
                                     Field::BinaryField, Field::DateTimeField, Field::ForeignKey,
                                     Field::DecimalField, std::shared_ptr<FieldAttr>
                                    >;
} //INFO: namespace psql

namespace db = psql;
#endif
