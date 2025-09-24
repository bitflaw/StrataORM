#include "../../include/strata/psql/create_model_header.hpp"
#include <fstream>

namespace psql{

void create_models_hpp(const ms_map& migrations){
  std::ofstream models_hpp("models.hpp");
  std::string cols_str {};

  if(!migrations.empty())
     models_hpp<<"#include <pqxx/row>\n\n";

  for(const auto& [model_name, col_map] : migrations){
    models_hpp<<"class " + model_name + "{\npublic:\n"
              <<"  std::string table_name = \"" + model_name + "\";\n  int id;\n";
    for(const auto& [col_name, dtv_obj] : col_map){
      cols_str += col_name + ",";
      std::visit([&](auto& col_obj){
        models_hpp<< "  " + col_obj->ctype + " " + col_name + ";\n";
      }, dtv_obj);
    }
    cols_str.pop_back();
    models_hpp<< "  std::vector<pqxx::row> records;\n"
      << "  std::string col_str = \"" + cols_str + "\";\n"
      << "  int col_map_size = " + std::to_string(col_map.size()) + ";\n\n"
      << "  " + model_name + "() = default;\n"
      << "  template <typename tuple_T>\n"
      << "  " + model_name + "(tuple_T tup){\n"
      << "    std::tie(id," + cols_str + ") = tup;\n  }\n\n"
      << "  auto get_attr() const{\n"
      << "    return std::make_tuple(id," + cols_str + ");\n  }\n};\n\n";
    cols_str.clear();
  }
}

}
