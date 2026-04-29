#include <strata/db_config.hpp>

#ifdef MARIADB

#include <strata/mariadb/create_constraints.hpp"
#include <strata/mariadb/datatypes.hpp"

namespace mariadb {

template<typename T>
FieldAttr& as_ref(T& obj)
{
    if constexpr (std::is_same_v<T, std::shared_ptr<FieldAttr>>) {
        return *obj;
    } else {
        return obj;
    }
}

template <typename Col_Map>
void create_table(const std::string& model_name, Col_Map& field_map, std::ofstream& Migrations)
{
  std::vector<std::string> primary_key_cols;
  std::vector<std::string> unique_constraint_cols;

  Migrations<< "CREATE TABLE IF NOT EXISTS " + model_name + " (\n  " + model_name + "_id SERIAL,\n  ";

  for(auto& [col, dtv_obj] : field_map){
    std::visit([&](auto& visited_col){
      using col_T = decltype(visited_col);
//TODO: create some sort of stash map for fk fields, then create them at the end of table.
      if constexpr(std::is_same_v<col_T, Field::ForeignKey>){
        Migrations << "  ";
        create_column(col, visited_col.sql_type, Migrations);
        Migrations << ",\n  ";
        create_fk_constraint(visited_col.sql_segment, col, Migrations);
        Migrations << ",\n  ";
        return;
      }

      FieldAttr& col_obj = as_ref(visited_col);
      if(col_obj.primary_key){
        primary_key_cols.push_back(col);
      }

      if(col_obj.unique){
        unique_constraint_cols.push_back(col);
      }

      create_column(col, col_obj.sql_segment, Migrations);
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

namespace db = mariadb;
#endif
