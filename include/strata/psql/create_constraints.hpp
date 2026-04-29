#pragma once
#include <strata/db_config.hpp>

#ifdef PSQL

#include <fstream>
#include <vector>
#include <variant>

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

}

namespace db = psql;
#endif
