#include <iostream>
#include <string>
#include <cstdlib>
#include <type_traits>
#include <variant>
#include <vector>
#include <fstream>
#include "../includes/models.hpp"
#include "../includes/datatypes.hpp"
#include "../../user/includes/renames.hpp"

using f_rename_m = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;
using m_rename_m = std::unordered_map<std::string, std::string>;

template <typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void create_model_hpp(const ms_map& migrations){
  std::ofstream models_hpp("model.hpp");
  std::string cols_str;
  models_hpp<<"#include <string>\n#include <vector>\n"
            << "#include <pqxx/row>\n\n";

  for(const auto& [model_name, col_map] : migrations){
    models_hpp<< "class " + model_name + "{\npublic:\n";
    for(const auto& [col_name, dtv_obj] : col_map){
      cols_str += col_name + ",";
      std::visit([&](auto& col_obj){
//TODO rn, foreign key is hardcoded with int datatype, add more for multiple cols, and also some support for easy access
        if constexpr(std::is_same_v<std::decay_t<decltype(col_obj)>, ForeignKey>){
          models_hpp<<"  int " + col_name + ";\n";
          return;
        }
        models_hpp<< "  " + col_obj.ctype + " " + col_name + ";\n";
      }, dtv_obj);
    }
    cols_str.pop_back();
    models_hpp<< "  std::vector<pqxx::row> records;\n"
              << "  std::string col_str = " + cols_str + ";\n\n"
              << "  " + model_name + "() = default;\n"
              << "  template <typename... Args>\n"
              << "  " + model_name + "(Args&&... args){\n"
              << "    std::tie(" + cols_str + ") = std::forward_as_tuple(std::forward<Args>(args)...);\n  }\n\n"
              << "  auto get_attr() const{\n"
              << "    return std::make_tuple(" + cols_str + ");\n  }\n};\n\n";
  }
}

bool is_file_empty(){
  bool is_empty = true;
  if(std::filesystem::exists("schema.json")){
    if(std::filesystem::file_size("schema.json") != 0){
      is_empty = false;
    }
  }
  return is_empty;
}

nlohmann::json jsonify(const ms_map& schema){
  nlohmann::json j;
  nlohmann::json j_col;
  for(const auto& [mn, fields] : schema){
    nlohmann::json field_json;
    for(const auto& [col, col_obj] : fields){
      variant_to_json(j_col, col_obj);
      field_json[col] = j_col;
    }
    j[mn] = field_json;
  }
  return j;
}

ms_map parse_to_obj(nlohmann::json& j){
  ms_map parsed;
  std::unordered_map<std::string, DataTypeVariant> fields;
  DataTypeVariant variant;
  for(const auto& [model, j_field_map] : j.items()){
    for(const auto& [col, json_dtv] : j_field_map.items()){
      variant_from_json(json_dtv, variant);
      fields[col] = variant;
    }
    parsed[model] = fields;
  }
  return parsed;
}

void save_schema_ms(const ms_map& schema){
  std::ofstream schema_ms_file("schema.json");
  if(!schema_ms_file.is_open()) throw std::runtime_error("Could not write schema into file.");
  schema_ms_file << jsonify(schema).dump(4);
}

ms_map load_schema_ms(){
  std::ifstream schema_ms_file("schema.json");
  if(!schema_ms_file.is_open()) throw std::runtime_error("Could not load schema from file.");
  nlohmann::json j;
  schema_ms_file >> j;
  return parse_to_obj(j);
}

void Model::make_migrations(){
  for(const auto& pair : ModelFactory::registry()){
    new_ms[pair.first] = ModelFactory::create_model_instance(pair.first)->fields;
  }
  if(!is_file_empty()){
    init_ms = load_schema_ms();
  }
  save_schema_ms(new_ms);
  track_changes();
  create_model_hpp(new_ms);
}

std::string sqlize_table(std::string model_name, std::unordered_map<std::string, DataTypeVariant>& fields){
  std::vector<std::string> primary_key_cols;
  std::vector<std::string> sql_strings;

  for(auto& [col, dtv_obj] : fields){
    std::visit([&](auto& col_obj){
      if constexpr(std::is_same_v<std::decay_t<decltype(col_obj)>, ForeignKey>){
        std::string fk_sql_seg = col_obj.sql_generator(col);
        //NOTE check the correct syntax and logic for foreign keys and whether they require columns
        fk_sql_seg = "CONSTRAINT fk_" + model_name + "_" + col + " " + fk_sql_seg;
        sql_strings.push_back(fk_sql_seg);
        return;
      }
      if(col_obj.primary_key){
        primary_key_cols.push_back(col);
      }

      if constexpr(std::is_same_v<std::decay_t<decltype(col_obj)>, IntegerField>){
        if(col_obj.check_condition != "default"){
          std::string cond = col_obj.check_condition;
          std::string sql_seg;

          /*if(cond == ">=")
            sql_seg = "CHECK (" + col + ">=" + std::to_string(col_obj.check_constraint) + ")";
          else if(cond == "<=")
            sql_seg = "CHECK (" + col + "<=" + std::to_string(col_obj.check_constraint) + ")";
          else if(cond == ">")
            sql_seg = "CHECK (" + col + ">" + std::to_string(col_obj.check_constraint) + ")";
          else if(cond == "<")
            sql_seg = "CHECK (" + col + "<" + std::to_string(col_obj.check_constraint) + ")";
          else if(cond == "!=")
            sql_seg = "CHECK (" + col + "!=" + std::to_string(col_obj.check_constraint) + ")";
          else if(cond == "=")
            sql_seg = "CHECK (" + col + "=" + std::to_string(col_obj.check_constraint) + ")";
          else
            std::cerr << "No such check condition " << cond << " was found" << std::endl;*/

          sql_strings.push_back(sql_seg);
        }
      }

      std::string full_sql_segment = col + " " + col_obj.sql_segment;
      sql_strings.push_back(full_sql_segment);

      if(col_obj.unique){
        std::string uq_constraint = "CONSTRAINT uq_" + col + " UNIQUE (" + col + ")";
        sql_strings.push_back(uq_constraint);
      }
    }, dtv_obj);
  }

  std::string pk_def_col = model_name + "_id SERIAL NOT NULL";
  sql_strings.push_back(pk_def_col);
  std::string pk_seg = "CONSTRAINT pk_" + model_name + " PRIMARY KEY (" + model_name + "_id)";
  if (!primary_key_cols.empty()) {
    pk_seg.replace(pk_seg.length() - 1, 1, ",");
    for(const auto& col : primary_key_cols) {
      pk_seg += col + ",";
    }
    pk_seg.replace(pk_seg.length() - 1, 1, ")");
  }
  sql_strings.push_back(pk_seg);

  std::string create_table_string = "CREATE TABLE " + model_name + " (";
  for(const auto& sql_str : sql_strings){
    create_table_string += sql_str + ",";
  }
  create_table_string.replace(create_table_string.length() - 1, 1, ");\n");

  return create_table_string;
}

std::vector<std::string> rename(m_rename_m& mrm, f_rename_m& frm, ms_map& init_ms){
  std::vector<std::string> renames;
  std::string rename;

  for(const auto& [old_mn, new_mn] : mrm){//rename operation for models using the model rename map(mrm)
    if(init_ms.find(old_mn) != init_ms.end()){
      init_ms[new_mn] = init_ms[old_mn];
      init_ms.erase(old_mn);
      rename = "ALTER TABLE " + old_mn + " RENAME TO " + new_mn + ";\n";
      renames.push_back(rename);
    }else{
      std::cerr<<"The model "<< old_mn <<" does not exist. Check for spelling mistakes in your model  rename map"<<std::endl;
    }
  }

  for(auto& [new_mn, col_renames] : frm){
    if(init_ms.find(new_mn) != init_ms.end()){
      for(auto& [old_cn, new_cn] : col_renames){
        if(init_ms[new_mn].find(old_cn) != init_ms[new_mn].end()){
          init_ms[new_mn][new_cn] = init_ms[new_mn][old_cn];
          init_ms[new_mn].erase(old_cn);
          rename = "ALTER TABLE " + new_mn + " RENAME COLUMN " + old_cn + " TO " + new_cn + ";\n" ;
          renames.push_back(rename);
        }else{
          std::cerr<<"Column name \""<< old_cn <<"\" passed into the column rename map."<<std::endl;
        }
      }
    }else{
      std::cerr<<"Invalid model name \""<< new_mn <<"\" in the column rename map. Check for spelling mistakes."<<std::endl;
    }
  }
  return renames;
} 

std::vector<std::string> create_or_drop_tables(ms_map& init_ms, ms_map& new_ms){
  std::vector<std::string> db_mods;
  std::string mods;
  char choice = 'n';

  for(const auto& [model, field_map] : init_ms){
    if(new_ms.find(model) == new_ms.end()){
      std::cout<<"The model "<<model<< " will be dropped. Are u sure about this?"<<std::endl;
      std::cin >>choice;
      if(choice == 'y' || choice == 'Y'){
        std::cout<<"The model "<<model<<" will be dropped from the database."<<std::endl;
        mods = "DROP TABLE " + model + ";\n";
        db_mods.push_back(mods);
        init_ms.erase(model);
      }
    }
  }

  for(auto& [model, field_map] : new_ms){
    if(init_ms.find(model) == init_ms.end()){
      std::string sql_create_table = sqlize_table(model, field_map);
      db_mods.push_back(sql_create_table);
      new_ms.erase(model);
    }
  }

  return db_mods;
}

void handle_types(ms_map::iterator& new_it, const std::string col, DataTypeVariant& dtv_obj, DataTypeVariant& init_dtv, std::ofstream& Migrations){
  std::string alterations = "";
  auto visitor = overloaded{
    [&](DateTimeField& col_obj){
      std::visit(overloaded{
        [&](DateTimeField& init_field){
          if(init_field.datatype != col_obj.datatype){
            alterations = col + " TYPE " + col_obj.datatype;
            Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }
          if((init_field.enable_default != col_obj.enable_default) && col_obj.enable_default){
            alterations = col + " SET DEFAULT " + col_obj.default_val;
            Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to DateTimeField are not compatible."<<std::endl;
          //convert_to_DateTimeField(col_obj, init_field);
        }
      }, init_dtv);
    },
    [&](IntegerField& col_obj){
      std::visit(overloaded{
        [&](IntegerField& init_field){
          if(init_field.datatype != col_obj.datatype){
            alterations = col + " TYPE "+ col_obj.datatype;
            Migrations << "ALTER TABLE " + new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }
          /*if((init_field.check_condition != col_obj.check_condition) && col_obj.check_condition != "default"){
                        string check = "CHECK(" + col + col_obj.check_condition + 
                                        std::to_string(col_obj.check_constraint) + ")";
                        Migrations << "ALTER TABLE " + new_it->first + " ALTER COLUMN " + alterations + ";\n";
                    }*/
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to IntegerField are not compatible."<<std::endl;
          //convert_to_IntegerField(col_obj, init_field);
        }
      }, init_dtv);
    },
    [&](ForeignKey& col_obj){
      std::string constraint_name = "fk_";
      for(auto& model_renames : model_renames){
        if(model_renames.second == new_it->first){
          constraint_name = constraint_name + model_renames.first;
        }else{
          constraint_name = constraint_name + new_it->first;
        }
      }
      for(auto& col_renames : column_renames){
        std::string cr_first = constraint_name.substr(3, std::string::npos);
        if(col_renames.first == cr_first){
          for(auto& col_rns : col_renames.second){
            if(col == col_rns.second){
              constraint_name = constraint_name + "_" + col_rns.first;
            }else{
              constraint_name = constraint_name + "_" + col;
            }
          }
        }else{
          constraint_name = constraint_name + "_" + col;
        }
      }
      //NOTE input some more logic here to handle if the maps are empty
      Migrations << "ALTER TABLE " + new_it->first + " DROP CONSTRAINT " + constraint_name + ";\n" + 
        "ALTER TABLE " + new_it->first + " ADD CONSTRAINT fk_" + new_it->first + "_" +
        col + " (" + col_obj.sql_generator(col)  + ");\n";
    },
    [&](DecimalField& col_obj){
      std::visit(overloaded{
        [&](DecimalField& init_field){
          if(init_field.datatype != col_obj.datatype){
            alterations = col + " TYPE " + col_obj.datatype + "(" + 
              std::to_string(col_obj.max_length) + std::to_string(col_obj.decimal_places) + ")";
            Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }

          if(init_field.max_length != col_obj.max_length || init_field.decimal_places != col_obj.decimal_places){
            alterations = col + " TYPE " + col_obj.datatype +"(" + 
              std::to_string(col_obj.max_length) + std::to_string(col_obj.decimal_places) + ")";
            Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to DecimalField are not compatible."<<std::endl;
          //convert_to_DecField(col_obj, init_field, col, model_name);
          //model_name and column name to check if the db table has data, and choose whether to convert if 
          //it doesn't or does
        }
      }, init_dtv);
    },
    [&](CharField& col_obj){
      std::visit(overloaded{
        [&](CharField& init_field){
          if((init_field.datatype != col_obj.datatype) || (init_field.length != col_obj.length)){
            alterations = col + " TYPE " + col_obj.datatype + "(" + std::to_string(col_obj.length) + ")";
            Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to CharField are not compatible."<<std::endl;
          //convert_to_CharField(col_obj, init_field);
        }
      }, init_dtv);
    },
    [&](BinaryField& col_obj){
      std::visit(overloaded{
        [&](BinaryField& init_field){
          if(init_field.size != col_obj.size){
            alterations = col + " TYPE BYTEA(" + std::to_string(col_obj.size) + ")";
            Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to BinaryField are not compatible."<<std::endl;
          //convert_to_BinaryField(col_obj, init_field)
        }
      }, init_dtv);
    },
    [&](BoolField& col_obj){
      std::visit(overloaded{
        [&](BoolField& init_field){
          if(init_field.enable_default != col_obj.enable_default){
            if(col_obj.enable_default){
              alterations = col + " SET DEFAULT TRUE";
              Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
            }else{
              alterations = col + " DROP DEFAULT";
              Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
            }
          }
        },
        [&](auto& init_field){
          std::cerr<<"Conversions from your datatype to BoolField are not compatible."<<std::endl;
          //convert_to_BoolField(col_obj itself, and the init_field for conversion compatibility checks);
        }
      }, init_dtv);
    }
  };
  std::visit(visitor, dtv_obj);
}

void Model::track_changes(){
  //TODO a replace the rename maps with json objects which are more lightweight
  m_rename_m mrm = model_renames;
  f_rename_m frm = column_renames;

  std::ofstream Migrations ("migrations.sql");
  if(init_ms.empty()){
    for(auto& [model_name, field_map] : new_ms){
      std::string sql_table_string = sqlize_table(model_name, field_map);
      Migrations << sql_table_string;
    }
    return;
  }

  for(auto& str : rename(mrm, frm, init_ms)) Migrations<< str;
  for(auto& str : create_or_drop_tables(init_ms, new_ms)) Migrations<< str; 

  std::vector<std::string> pk_cols;
  std::string alterations, pk, fk;

  for(auto& [init_model_name, init_col_map]:init_ms){
    auto new_it = new_ms.find(init_model_name);
    if(new_it == new_ms.end()){
      std::cerr<<"Something went wrong in the check for the new iterator"<<std::endl;
      return;
    }
    for(auto& [new_col, dtv_obj] : new_it->second){
      std::visit([&](auto& col_obj){
        if(init_col_map.find(new_col) == init_col_map.end()){
          Migrations << "ALTER TABLE " + new_it->first + " ADD " + new_col + " " + col_obj.sql_segment + ";\n";
          return;
        }
        std::visit([&](auto& init_field){
          if constexpr(std::is_same_v<std::decay_t<decltype(col_obj)>, ForeignKey>){
            col_obj.sql_generator(new_col);
          }

          if(init_field.sql_segment != col_obj.sql_segment){

            handle_types(new_it, new_col, dtv_obj, init_col_map[new_col], Migrations);
            if(col_obj.primary_key){
              pk_cols.push_back(new_col);
            }

            if(init_field.not_null != col_obj.not_null){
              if(col_obj.not_null){
                alterations = new_col + " SET NOT NULL";
                Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + alterations + ";\n";
              }else{
                Migrations << "ALTER TABLE " +  new_it->first + " ALTER COLUMN " + new_col + " DROP NOT NULL;\n";
              }
            }

            if(init_field.unique != col_obj.unique){
              if(col_obj.unique){
                std::string constraint_name = "uq";
                for(auto& [m_nms, col_map] : frm){
                  if(m_nms == new_it->first){
                    for(auto& [old_cn, new_cn] : col_map){
                      if(new_col == new_cn){
                        constraint_name = constraint_name + "_" + old_cn;
                      }else{
                        constraint_name = constraint_name + "_" + new_col;
                      }
                    }
                  }else{
                    constraint_name = constraint_name + "_" + new_col;
                  }
                }
                if(frm.empty()){
                  Migrations << "ALTER TABLE " + new_it->first + " ADD CONSTRAINT uq_" + new_col + " UNIQUE(" + new_col + ");\n";
                }else{
                  Migrations << "ALTER TABLE " + new_it->first + " ADD CONSTRAINT uq_" + new_col + " UNIQUE(" + new_col + ");\n";
                }

              }else{
                std::string constraint_name = "uq";
                for(auto& [m_nms, col_map] : frm){
                  if(m_nms == new_it->first){
                    for(auto& [old_cn, new_cn] : col_map){
                      if(new_col == new_cn){
                        constraint_name = constraint_name + "_" + old_cn;
                      }else{
                        constraint_name = constraint_name + "_" + new_col;
                      }
                    }
                  }else{
                    constraint_name = constraint_name + "_" + new_col;
                  }
                }
                if(frm.empty()){
                  Migrations << "ALTER TABLE " + new_it->first + " DROP CONSTRAINT uq_" + new_col + ";\n";
                }else{
                  Migrations << "ALTER TABLE " + new_it->first + " DROP CONSTRAINT " + constraint_name + ";\n";
                }
              }
            }
          }
        }, init_col_map[std::string(new_col)]);
      }, dtv_obj);

      if(!mrm.empty()){
        std::string pk_constraint = "pk_";
        for(auto& [old_mn, new_mn] : mrm){
          if(new_mn == new_it->first){
            pk_constraint += old_mn;
          }else{
            pk_constraint += new_it->first;
          }
        }
        Migrations << "ALTER TABLE " + new_it->first + " DROP CONSTRAINT " + pk_constraint + ";\n";
      }

      if(pk_cols.size() != 0){
        std::string pk_seg = "pk_" + new_it->first + " PRIMARY KEY (" + new_it->first + "_id,";
        for(auto& col : pk_cols)pk_seg += col + ",";
        pk_seg.replace(pk_seg.length()-1, 1, ")");

        Migrations << "ALTER TABLE " + new_it->first + " ADD CONSTRAINT " + pk_seg + ";\n";
      }
    }
  }

  //ERROR undefined behaviour from this for loop. The init_it iterator somehow accesses the columns to other tables
  //leading to undefined drops of these columns even though they are not to be be dropped...

  for(const auto& [new_model_name, new_col_map]:new_ms){
    const auto init_it = init_ms.find(new_model_name);
    if(init_it == init_ms.end()){
      std::cerr<<"Something went wrong in the check for the init iterator."<<std::endl;
      return;
    }
    for(auto& [old_col, dtv_obj] : init_it->second){
      std::cout<<"Col in init_ms is: "<<old_col<<"in model: "<< init_it->first<<std::endl;
      if(new_col_map.find(old_col) == new_col_map.end()){
        std::cout<<"Dropping col: "<<old_col<<std::endl;
        Migrations << "ALTER TABLE " + new_model_name + " DROP " + old_col + ";\n";
      }
    }
  }
}
