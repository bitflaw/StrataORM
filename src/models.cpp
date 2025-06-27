#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <fstream>
#include "../strata/models.hpp"
#include "../strata/db_adapters.hpp"

template <typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

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
    fields.clear();
  }

  return parsed;
}

void save_schema_ms(const ms_map& schema){
  std::ofstream schema_ms_file("schema.json");
  if(!schema_ms_file.is_open()) throw std::runtime_error("[ERROR: from 'save_schema_ms()'] => Could not write schema into file.");
  schema_ms_file << jsonify(schema).dump(2);
}

ms_map load_schema_ms(){
  std::ifstream schema_ms_file("schema.json");
  if(!schema_ms_file.is_open()) throw std::runtime_error("[ERROR: from 'load_schema_ms()'] => Could not load schema from file.");
  nlohmann::json j;
  schema_ms_file >> j;
  return parse_to_obj(j);
}

void Model::make_migrations(const nlohmann::json& mrm, const nlohmann::json& frm, std::string sql_filename){
  for(const auto& pair : ModelFactory::registry()){
    new_ms[pair.first] = ModelFactory::create_model_instance(pair.first)->col_map;
  }
  if(std::filesystem::exists("schema.json") && std::filesystem::file_size("schema.json") > 0){
    init_ms = load_schema_ms();
  }
  save_schema_ms(new_ms);
  track_changes(mrm, frm, sql_filename);
  db_adapter::create_models_hpp(new_ms);
}

void rename(const nlohmann::json& mrm, const nlohmann::json& frm, ms_map& init_ms, std::ofstream& Migrations){

  for(const auto& [old_mn, new_mn] : mrm.items()){
    if(init_ms.find(old_mn) != init_ms.end()){
      init_ms[new_mn.get<std::string>()] = init_ms[old_mn];
      db_adapter::alter_rename_table(old_mn, new_mn.get<std::string>(), Migrations);
      init_ms.erase(old_mn);
    }else{
      throw std::runtime_error(R"([ERROR: from 'rename()' inside model renames]=>
                                Invalid model name passed into the model rename map.
                                Check for spelling mistakes)");
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
          throw std::runtime_error(R"([ERROR: from 'rename()' in column renames]=>
                                    The (old)column you passed in to the frm doesn't exist in the migrations.
                                    Check for spelling mistakes.)");
        }
      }
    }else{
      throw std::runtime_error(R"([ERROR: from 'rename()' inside column renames]=>
                                Invalid model name passed into the field rename map.
                                Check for spelling mistakes)");
    }
  }
}

void create_or_drop_tables(ms_map& init_ms, ms_map& new_ms, std::ofstream& Migrations){
  char choice = 'n';

  for(auto it = init_ms.begin(); it != init_ms.end();){
    const auto& [model, field_map] = *it;
    if(new_ms.find(model) == new_ms.end()){
      std::cout<<"The model "<<model<< " will be dropped. Are you sure about this?(y or n)"<<std::endl;
      std::cin >>choice;
      if(choice == 'y' || choice == 'Y'){
        std::cout<<"The model "<<model<<" will be dropped from the database."<<std::endl;
        db_adapter::drop_table(model, Migrations);
        it = init_ms.erase(it);
        continue;
      }
    }
    ++it;
  }

  for(auto it = new_ms.begin(); it != new_ms.end();){
    const auto& [model, field_map] = *it;
    if(init_ms.find(model) == init_ms.end()){
      db_adapter::create_table(model, field_map, Migrations);
      it = new_ms.erase(it);
      continue;
    }
    ++it;
  }
}

void handle_types(ms_map::iterator& new_it, const std::string col, DataTypeVariant& dtv_obj, const nlohmann::json& mrm,
                  const nlohmann::json& frm, DataTypeVariant& init_dtv, std::ofstream& Migrations){
  std::string alterations;

  auto visitor = overloaded{
    [&](std::shared_ptr<DateTimeField>& col_obj){
      std::visit(overloaded{
        [&](std::shared_ptr<DateTimeField>& init_field){
          if(init_field->datatype != col_obj->datatype){
            db_adapter::alter_column_type(new_it->first, col, col_obj->datatype, Migrations);
          }
          if((init_field->enable_default != col_obj->enable_default) && col_obj->enable_default){
            db_adapter::alter_column_defaultval(new_it->first, col, true, col_obj->default_val, Migrations);
          }else{
            db_adapter::alter_column_defaultval(new_it->first, col, false, col_obj->default_val, Migrations);
          }
          return;
        },
        [&](auto& init_field){
          std::runtime_error(R"([ERROR: in 'handle_types.DateTimeField lambda()'] => 
                             Conversions of from the defined type to DateTimeField are not compatible.)");
          //convert_to_DateTimeField(col_obj, init_field);
          return;
        }
      }, init_dtv);
    },
    [&](std::shared_ptr<IntegerField>& col_obj){
      std::visit(overloaded{
        [&](std::shared_ptr<IntegerField>& init_field){
          if(init_field->datatype != col_obj->datatype){
            db_adapter::alter_column_type(new_it->first, col, col_obj->datatype, Migrations);
          }
        /*if((init_field.check_condition != col_obj.check_condition) && col_obj.check_condition != "default"){
            string check = "CHECK(" + col + col_obj.check_condition + std::to_string(col_obj.check_constraint) + ")";
            Migrations << "ALTER TABLE " + new_it->first + " ALTER COLUMN " + alterations + ";\n";
          }*/
          return;
        },
        [&](auto& init_field){
          std::runtime_error(R"([ERROR: in 'handle_types.IntegerField lambda()'] => 
                             Conversions of from the defined type to IntegerField are not compatible.)");
          //convert_to_IntegerField(col_obj, init_field);
          return;
        }
      }, init_dtv);
    },
    [&](std::shared_ptr<ForeignKey>& col_obj){
      std::string constraint_name = "fk_";
      for(auto& [model_name, col_renames]: frm.items()){
        if(model_name == new_it->first){
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
      db_adapter::drop_constraint(new_it->first, constraint_name, Migrations);
      Migrations<<"ALTER TABLE " + new_it->first + " ADD ";
      db_adapter::create_fk_constraint(new_it->first, col_obj->sql_segment, col, Migrations);
      return;
    },
    [&](std::shared_ptr<DecimalField>& col_obj){
      std::visit(overloaded{
        [&](std::shared_ptr<DecimalField>& init_field){
          if(init_field->datatype != col_obj->datatype ||
             init_field->max_length != col_obj->max_length ||
             init_field->decimal_places != col_obj->decimal_places){

            alterations = col_obj->datatype + " (" + std::to_string(col_obj->max_length) + "," + std::to_string(col_obj->decimal_places) + ")";
            db_adapter::alter_column_type(new_it->first, col, alterations, Migrations);
          }
          return;
        },
        [&](auto& init_field){
          std::runtime_error(R"([ERROR: in 'handle_types.DecimalField lambda()'] => 
                             Conversions of from the defined type to DecimalField are not compatible.)");
          //convert_to_DecField(col_obj, init_field, col, model_name);
          return;
        }
      }, init_dtv);
    },
    [&](std::shared_ptr<CharField>& col_obj){
      std::visit(overloaded{
        [&](std::shared_ptr<CharField>& init_field){
          if((init_field->datatype != col_obj->datatype) || (init_field->length != col_obj->length)){
            alterations = "VARCHAR( " + std::to_string(col_obj->length) + " )";
            db_adapter::alter_column_type(new_it->first, col, alterations, Migrations);
          }
          return;
        },
        [&](auto& init_field){
          std::runtime_error(R"([ERROR: in 'handle_types.CharField lambda()'] => 
                             Conversions of from the defined type to CharField are not compatible.)");
          //convert_to_CharField(col_obj, init_field);
          return;
        }
      }, init_dtv);
    },
    [&](std::shared_ptr<BinaryField>& col_obj){
      std::visit(overloaded{
        [&](std::shared_ptr<BinaryField>& init_field){
          return;
        },
        [&](auto& init_field){
          std::runtime_error(R"([ERROR: in 'handle_types.BinaryField lambda()'] => 
                             Conversions of from the defined type to BinaryField are not compatible.)");
          //convert_to_BinaryField(col_obj, init_field)
          return;
        }
      }, init_dtv);
    },
    [&](std::shared_ptr<BoolField>& col_obj){
      std::visit(overloaded{
        [&](std::shared_ptr<BoolField>& init_field){
          if(init_field->enable_default != col_obj->enable_default){
            if(col_obj->enable_default){
              db_adapter::alter_column_defaultval(new_it->first, col, true, std::to_string(col_obj->default_value), Migrations);
            }else{
              alterations = col + " DROP DEFAULT";
              db_adapter::alter_column_defaultval(new_it->first, col, false, "false", Migrations);
            }
          }
          return;
        },
        [&](auto& init_field){
          std::runtime_error(R"([ERROR: in 'handle_types.BoolField lambda()'] => 
                             Conversions of from the defined type to BoolField are not compatible.)");
          //convert_to_BoolField(col_obj itself, and the init_field for conversion compatibility checks);
          return;
        }
      }, init_dtv);
    }
  };
  std::visit(visitor, dtv_obj);
}

std::string find_uq_constraint(const nlohmann::json& frm, const std::string& new_model_name, const std::string& new_col){
  std::string constraint_name;
  auto outer_it = frm.find(new_model_name);
  if(outer_it != frm.end()){
    for(auto& [old_cn, new_cn] : outer_it.value().items()){
      if(new_col == new_cn.get<std::string>()){
        constraint_name = old_cn;
      }else{
        constraint_name = new_col;
      }
    }
  }else{
    constraint_name = new_col;
  }
  return constraint_name;
}

void Model::track_changes(const nlohmann::json& mrm, const nlohmann::json& frm, std::string sql_filename){

  std::ofstream Migrations (sql_filename);

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

  for(auto& [init_model_name, init_col_map] : init_ms){
    auto new_it = new_ms.find(init_model_name);
    if(new_it == new_ms.end()){
      throw std::runtime_error("[ERROR: in 'track_changes()'] => Error in check for new iterator with initial model name against new_ms.");
    }
    for(auto& [new_col, dtv_obj] : new_it->second){
      std::visit([&](auto& col_obj){
        if(init_col_map.find(new_col) == init_col_map.end()){
          db_adapter::alter_add_column(new_it->first, new_col, col_obj->sql_segment, Migrations);
          return;
        }
        std::visit([&](auto& init_field){
          if(init_field->sql_segment != col_obj->sql_segment){
            handle_types(new_it, new_col, dtv_obj, mrm, frm, init_col_map[new_col], Migrations);

            if(col_obj->primary_key){
              pk_cols.push_back(new_col);
            }

            if(init_field->not_null != col_obj->not_null){
              if(col_obj->not_null){
                db_adapter::alter_column_nullable(new_it->first, new_col, false, Migrations);
              }else{
                db_adapter::alter_column_nullable(new_it->first, new_col, true, Migrations);
              }
            }

            if((init_field->unique != col_obj->unique) && col_obj->unique){
              if(frm.empty()){
                uq_cols.push_back(new_col);
              }else{
                uq_cols.push_back(find_uq_constraint(frm, new_it->first, new_col));
              }
            }else if((init_field->unique != col_obj->unique) && !col_obj->unique){
              if(frm.empty()){
                db_adapter::drop_constraint(new_it->first, "uq_"+new_col , Migrations);
              }else{
                db_adapter::drop_constraint(new_it->first, "uq_"+find_uq_constraint(frm,new_it->first,new_col), Migrations);
              }
            }else {
              return;
            }
          }
        }, init_col_map[std::string(new_col)]);
      }, dtv_obj);

      for(const std::string& uq_col : uq_cols){
        Migrations<<"ALTER TABLE " + new_it->first + " ADD ";
        db_adapter::create_uq_constraint(uq_col, Migrations);
      }
      uq_cols.clear();

      if(!mrm.empty() && !pk_cols.empty()){
        std::string pk_constraint = "pk_";
        for(auto& [old_mn, new_mn] : mrm.items()){
          if(new_mn.get<std::string>() == new_it->first){
            pk_constraint += old_mn;
          }else{
            pk_constraint += new_it->first;
          }
        }
        db_adapter::drop_constraint(new_it->first, pk_constraint, Migrations);
      }else if(mrm.empty() && !pk_cols.empty()){
        db_adapter::drop_constraint(new_it->first, "pk_" + new_it->first , Migrations);
      }else{
        continue;
      }

      if(!pk_cols.empty()){
        Migrations<<"ALTER TABLE " + new_it->first + " ADD ";
        db_adapter::create_pk_constraint(new_it->first, pk_cols, Migrations);
        Migrations<<";\n";
      }
      pk_cols.clear();
    }
  }

  for(const auto& [new_model_name, new_col_map]:new_ms){
    const auto init_it = init_ms.find(new_model_name);
    if(init_it == init_ms.end()){
      throw std::runtime_error(R"([ERROR: in 'track_changes()'] =>
                                  Error in check for init iterator with new model name against the initial migrations.)");
    }
    for(auto& [old_col, dtv_obj] : init_it->second){
      if(new_col_map.find(old_col) == new_col_map.end()){
        db_adapter::drop_column(init_it->first, old_col, Migrations);
      }
    }
  }
}
