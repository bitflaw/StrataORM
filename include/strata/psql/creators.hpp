#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <variant>
#include "../datatypes.hpp"
#include "../db_config.hpp"

#ifdef PSQL

namespace psql {

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

inline void create_fk_constraint(const std::string& fk_sql_segment, const std::string& column_name,
                                 std::ofstream& Migrations)
{
  Migrations<< "CONSTRAINT fk_" + column_name + " " + fk_sql_segment;
}

inline void create_uq_constraint(const std::string& uq_col, std::ofstream& Migrations)
{
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
        create_fk_constraint(col_obj->sql_segment, col, Migrations);
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

}

namespace db_adapter = psql;
#endif
