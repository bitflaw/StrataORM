#include <iostream>
#include <string>
#include <cstdlib>
#include <type_traits>
#include <variant>
#include <vector>
#include <fstream>
#include "../includes/models.hpp"

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

void Model::make_migrations(const nlohmann::json& mrm, const nlohmann::json& frm){
  for(const auto& pair : ModelFactory::registry()){
    new_ms[pair.first] = ModelFactory::create_model_instance(pair.first)->col_map;
  }
  if(!is_file_empty()){
    init_ms = load_schema_ms();
  }
  save_schema_ms(new_ms);
  track_changes(mrm, frm);
  create_model_hpp(new_ms);
}

void rename(const nlohmann::json& mrm, const nlohmann::json& frm, ms_map& init_ms, std::ofstream& Migrations){

  for(const auto& [old_mn, new_mn] : mrm.items()){
    if(init_ms.find(old_mn) != init_ms.end()){
      init_ms[new_mn.get<std::string>()] = init_ms[old_mn];
      db_adapter::alter_rename_table(old_mn, new_mn.get<std::string>(), Migrations);
      init_ms.erase(old_mn);
    }else{
      std::cerr<<"The model "<< old_mn <<" does not exist. Check for spelling mistakes in your model rename map"<<std::endl;
    }
  }

  for(const auto& [new_mn, col_renames] : frm.items()){
    if(init_ms.find(new_mn) != init_ms.end()){
      for(const auto& [old_cn, new_cn] : col_renames.items()){
        if(init_ms[new_mn].find(old_cn) != init_ms[new_mn].end()){
          init_ms[new_mn][new_cn.get<std::string>()] = init_ms[new_mn][old_cn];
          db_adapter::alter_rename_column(new_mn, old_cn, new_cn.get<std::string>(), Migrations);
          init_ms[new_mn].erase(old_cn);
        }else{
          std::cerr<<"Column name \""<< old_cn <<"\" passed into the column rename map."<<std::endl;
        }
      }
    }else{
      std::cerr<<"Invalid model name \""<< new_mn <<"\" in the column rename map. Check for spelling mistakes."<<std::endl;
    }
  }
}

void create_or_drop_tables(ms_map& init_ms, ms_map& new_ms, std::ofstream& Migrations){
  char choice = 'n';

  for(auto& [model, field_map] : init_ms){
    if(new_ms.find(model) == new_ms.end()){
      std::cout<<"The model "<<model<< " will be dropped. Are u sure about this?"<<std::endl;
      std::cin >>choice;
      if(choice == 'y' || choice == 'Y'){
        std::cout<<"The model "<<model<<" will be dropped from the database."<<std::endl;
        db_adapter::drop_table(model, Migrations);
        init_ms.erase(model);
      }
    }
  }

  for(auto& [model, field_map] : new_ms){
    if(init_ms.find(model) == init_ms.end()){
      db_adapter::create_table(model, field_map, Migrations);
      new_ms.erase(model);
    }
  }
}

void handle_types(ms_map::iterator& new_it, const std::string col, DataTypeVariant& dtv_obj, const nlohmann::json& mrm,
                  const nlohmann::json& frm, DataTypeVariant& init_dtv, std::ofstream& Migrations){
  std::string alterations;

  auto visitor = overloaded{
    [&](DateTimeField& col_obj){
      std::visit(overloaded{
        [&](DateTimeField& init_field){
          if(init_field.datatype != col_obj.datatype){
            db_adapter::alter_column_type(new_it->first, col, col_obj.datatype, Migrations);
          }
          if((init_field.enable_default != col_obj.enable_default) && col_obj.enable_default){
            db_adapter::alter_column_defaultval(new_it->first, col, true, col_obj.default_val, Migrations);
          }else{
            db_adapter::alter_column_defaultval(new_it->first, col, false, col_obj.default_val, Migrations);
          }
          return;
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to DateTimeField are not compatible."<<std::endl;
          //convert_to_DateTimeField(col_obj, init_field);
          return;
        }
      }, init_dtv);
    },
    [&](IntegerField& col_obj){
      std::visit(overloaded{
        [&](IntegerField& init_field){
          if(init_field.datatype != col_obj.datatype){
            db_adapter::alter_column_type(new_it->first, col, col_obj.datatype, Migrations);
          }
        /*if((init_field.check_condition != col_obj.check_condition) && col_obj.check_condition != "default"){
            string check = "CHECK(" + col + col_obj.check_condition + std::to_string(col_obj.check_constraint) + ")";
            Migrations << "ALTER TABLE " + new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }*/
          return;
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to IntegerField are not compatible."<<std::endl;
          //convert_to_IntegerField(col_obj, init_field);
          return;
        }
      }, init_dtv);
    },
    [&](ForeignKey& col_obj){
      std::string constraint_name = "fk_";
      for(auto& [old_mn, new_mn] : mrm.items()){
        if(new_mn.get<std::string>() == new_it->first){
          constraint_name = constraint_name + old_mn;
        }else{
          constraint_name = constraint_name + new_it->first;
        }
      }
      for(auto& [model_name, col_renames]: frm.items()){
        std::string cr_first = constraint_name.substr(3, std::string::npos);
        if(model_name == cr_first){
          for(auto& [old_cn, new_cn] : col_renames.items()){
            if(col == new_cn.get<std::string>()){
              constraint_name = constraint_name + "_" + old_cn;
            }else{
              constraint_name = constraint_name + "_" + col;
            }
          }
        }else{
          constraint_name = constraint_name + "_" + col;
        }
      }
      //NOTE input some more logic here to handle if the maps are empty
      db_adapter::drop_constraint(new_it->first, constraint_name, Migrations);
      db_adapter::create_fk_constraint(new_it->first, col_obj.sql_segment, col, Migrations);
      return;
    },
    [&](DecimalField& col_obj){
      std::visit(overloaded{
        [&](DecimalField& init_field){
          if(init_field.datatype != col_obj.datatype ||
             init_field.max_length != col_obj.max_length ||
             init_field.decimal_places != col_obj.decimal_places){

            alterations = col_obj.datatype + " (" + std::to_string(col_obj.max_length) + "," + 
                          std::to_string(col_obj.decimal_places) + ")";
            db_adapter::alter_column_type(new_it->first, col, alterations, Migrations);
          }
          return;
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to DecimalField are not compatible."<<std::endl;
          //convert_to_DecField(col_obj, init_field, col, model_name);
          return;
        }
      }, init_dtv);
    },
    [&](CharField& col_obj){
      std::visit(overloaded{
        [&](CharField& init_field){
          if((init_field.datatype != col_obj.datatype) || (init_field.length != col_obj.length)){
            alterations = "VARCHAR( " + std::to_string(col_obj.length) + " )";
            db_adapter::alter_column_type(new_it->first, col, alterations, Migrations);
          }
          return;
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to CharField are not compatible."<<std::endl;
          //convert_to_CharField(col_obj, init_field);
          return;
        }
      }, init_dtv);
    },
    [&](BinaryField& col_obj){
      std::visit(overloaded{
        [&](BinaryField& init_field){
          if(init_field.size != col_obj.size){
            alterations = "BYTEA(" + std::to_string(col_obj.size) + ")";
            db_adapter::alter_column_type(new_it->first, col, alterations, Migrations);
          }
          return;
        },
        [&](auto& init_field){
          std::cerr<<"Conversions of from the defined type to BinaryField are not compatible."<<std::endl;
          //convert_to_BinaryField(col_obj, init_field)
          return;
        }
      }, init_dtv);
    },
    [&](BoolField& col_obj){
      std::visit(overloaded{
        [&](BoolField& init_field){
          if(init_field.enable_default != col_obj.enable_default){
            if(col_obj.enable_default){
              db_adapter::alter_column_defaultval(new_it->first, col, true, std::to_string(col_obj.default_value), Migrations);
            }else{
              alterations = col + " DROP DEFAULT";
              db_adapter::alter_column_defaultval(new_it->first, col, false, "false", Migrations);
            }
          }
          return;
        },
        [&](auto& init_field){
          std::cerr<<"Conversions from your datatype to BoolField are not compatible."<<std::endl;
          //convert_to_BoolField(col_obj itself, and the init_field for conversion compatibility checks);
          return;
        }
      }, init_dtv);
    }
  };
  std::visit(visitor, dtv_obj);
}

void Model::track_changes(const nlohmann::json& mrm, const nlohmann::json& frm){

  std::ofstream Migrations ("migrations.sql");
  if(init_ms.empty()){
    for(auto& [model_name, field_map] : new_ms){
      db_adapter::create_table(model_name, field_map, Migrations);
    }
    return;
  }

  rename(mrm, frm, init_ms, Migrations);
  create_or_drop_tables(init_ms, new_ms, Migrations);

  std::vector<std::string> pk_cols, uq_cols;
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
          db_adapter::alter_add_column(new_it->first, new_col, col_obj.sql_segment, Migrations);
          return;
        }
        std::visit([&](auto& init_field){
          if(init_field.sql_segment != col_obj.sql_segment){
            handle_types(new_it, new_col, dtv_obj, mrm, frm, init_col_map[new_col], Migrations);

            if(col_obj.primary_key){
              pk_cols.push_back(new_col);
            }

            if(init_field.not_null != col_obj.not_null){
              if(col_obj.not_null){
                db_adapter::alter_column_nullable(new_it->first, new_col, false, Migrations);
              }else{
                db_adapter::alter_column_nullable(new_it->first, new_col, true, Migrations);
              }
            }

            if(init_field.unique != col_obj.unique){
              if(col_obj.unique){
                std::string constraint_name = "uq";
                for(auto& [m_nms, col_map] : frm.items()){
                  if(m_nms == new_it->first){
                    for(auto& [old_cn, new_cn] : col_map.items()){
                      if(new_col == new_cn.get<std::string>()){
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

                uq_cols.push_back(new_col);
              }else{
                std::string constraint_name = "uq";
                for(auto& [m_nms, col_map] : frm.items()){
                  if(m_nms == new_it->first){
                    for(auto& [old_cn, new_cn] : col_map.items()){
                      if(new_col == new_cn.get<std::string>()){
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
                  db_adapter::drop_constraint(new_it->first, "uq_"+new_col , Migrations);
                }else{
                  Migrations << "ALTER TABLE " + new_it->first + " DROP CONSTRAINT " + constraint_name + ";\n";
                  db_adapter::drop_constraint(new_it->first, constraint_name, Migrations);
                }
              }
            }
          }
        }, init_col_map[std::string(new_col)]);
      }, dtv_obj);

      for(const std::string& uq_col : uq_cols) db_adapter::create_uq_constraint(uq_col, Migrations);

      if(!mrm.empty()){
        std::string pk_constraint = "pk_";
        for(auto& [old_mn, new_mn] : mrm.items()){
          if(new_mn.get<std::string>() == new_it->first){
            pk_constraint += old_mn;
          }else{
            pk_constraint += new_it->first;
          }
        }
        db_adapter::drop_constraint(new_it->first, pk_constraint, Migrations);
      }else{
        db_adapter::drop_constraint(new_it->first, "pk_" + new_it->first , Migrations);
      }

      db_adapter::create_pk_constraint(new_it->first, pk_cols, Migrations);
      return;
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
      std::cout<<"Col in init_ms is: "<<old_col<<" in model: "<< init_it->first<<std::endl;
      if(new_col_map.find(old_col) == new_col_map.end()){
        std::cout<<"Dropping col: "<<old_col<<std::endl;
        db_adapter::drop_column(new_model_name, old_col, Migrations);
      }
    }
  }
}
