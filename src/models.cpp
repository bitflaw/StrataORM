#include "../include/strata/models.hpp"
#include "../include/strata/db_adapters.hpp"
#include <iostream>

template <typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

nlohmann::json jsonify(const ms_map& schema)
{
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

ms_map parse_to_obj(nlohmann::json& j)
{
  ms_map parsed {};
  fields col_fields {};
  db::DataTypeVariant variant {};

  for(const auto& [model, j_field_map] : j.items()){
    for(const auto& [col, json_dtv] : j_field_map.items()){
      variant_from_json(json_dtv, variant);
      col_fields[col] = variant;
    }
    parsed[model] = col_fields;
    col_fields.clear();
  }

  return parsed;
}

void save_schema_ms(const ms_map& schema)
{
  std::ofstream schema_ms_file("schema.json");
  if(!schema_ms_file.is_open()) throw std::runtime_error("[ERROR: from 'save_schema_ms()'] => Could not write schema into file.");
  schema_ms_file << jsonify(schema).dump(2);
}

ms_map load_schema_ms()
{
  std::ifstream schema_ms_file("schema.json");
  if(!schema_ms_file.is_open()) throw std::runtime_error("[ERROR: from 'load_schema_ms()'] => Could not load schema from file.");
  nlohmann::json j;
  schema_ms_file >> j;
  return parse_to_obj(j);
}

void Model::make_migrations(const nlohmann::json& mrm, const nlohmann::json& frm, std::string sql_filename)
{
  for(const auto& pair : ModelFactory::registry()){
    new_ms[pair.first] = ModelFactory::create_model_instance(pair.first)->col_map;
  }
  if(std::filesystem::exists("schema.json") && std::filesystem::file_size("schema.json") > 0){
    init_ms = load_schema_ms();
  }
  save_schema_ms(new_ms);
  track_changes(mrm, frm, sql_filename);
  db::create_models_hpp(new_ms);
}

void rename(const nlohmann::json& mrm, const nlohmann::json& frm, ms_map& init_ms, std::ofstream& Migrations)
{

  for(const auto& [old_mn, new_mn] : mrm.items()){
    if(init_ms.find(old_mn) != init_ms.end()){
      init_ms[new_mn.get<std::string>()] = init_ms[old_mn];
      db::alter_rename_table(old_mn, new_mn.get<std::string>(), Migrations);
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
          db::alter_rename_column(new_mn, old_cn, new_cn.get<std::string>(), Migrations);
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

void create_or_drop_tables(ms_map& init_ms, ms_map& new_ms, std::ofstream& Migrations)
{
  char choice = 'n';

  for(auto it = init_ms.begin(); it != init_ms.end();){
    const auto& [model, field_map] = *it;
    if(new_ms.find(model) == new_ms.end()){
      std::cout<<"The model "<<model<< " will be dropped. Are you sure about this?(y or n)"<<std::endl;
      std::cin >>choice;
      if(choice == 'y' || choice == 'Y'){
        std::cout<<"The model "<<model<<" will be dropped from the database."<<std::endl;
        db::drop_table(model, Migrations);
        it = init_ms.erase(it);
        continue;
      }
    }
    ++it;
  }

  for(auto it = new_ms.begin(); it != new_ms.end();){
    auto& [model, field_map] = *it;
    if(init_ms.find(model) == init_ms.end()){
      db::create_table(model, field_map, Migrations);
      it = new_ms.erase(it);
      continue;
    }
    ++it;
  }
}

template <typename new_col_T, typename old_col_T>
void handle_types(std::string new_model_name, const std::string col, FieldAttr& new_col_obj,
                  const nlohmann::json& frm, FieldAttr& old_col_obj, std::ofstream& Migrations)
{
  if constexpr(!std::is_same_v<new_col_T, old_col_T>)
  { throw std::logic_error("Conversions are not yet supported!"); }

  if constexpr(std::is_same_v<new_col_T, db::Field::ForeignKey>)
  {
    new_col_obj.track(new_model_name, col, old_col_obj, Migrations, frm);
    return;
  }

  new_col_obj.track(new_model_name, col, old_col_obj, Migrations);
}

std::string find_uq_constraint(const nlohmann::json& frm, const std::string& new_model_name, const std::string& new_col)
{
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

void check_for_column_drops(ms_map& new_ms, ms_map& init_ms, std::ofstream& Migrations)
{
  for(const auto& [new_model_name, new_col_map]:new_ms){
    const auto init_it = init_ms.find(new_model_name);
    if(init_it == init_ms.end()){
      throw std::runtime_error(R"([ERROR: in 'track_changes()'] =>
                               Error in check for init iterator with new model name against the initial migrations.)");
    }
    for(auto& [old_col, dtv_obj] : init_it->second){
      if(new_col_map.find(old_col) == new_col_map.end()){
        db::drop_column(init_it->first, old_col, Migrations);
      }
    }
  }
}

void Model::track_changes(const nlohmann::json& mrm, const nlohmann::json& frm, std::string sql_filename)
{
  std::ofstream Migrations (sql_filename);
  if(init_ms.empty())
  {
    for(auto& [model_name, field_map] : new_ms)
      db::create_table(model_name, field_map, Migrations);
    return;
  }

  rename(mrm, frm, init_ms, Migrations);
  create_or_drop_tables(init_ms, new_ms, Migrations);

  std::vector<std::string> pk_cols, uq_cols;
  std::string alterations, pk, fk;

  for(auto& [init_model_name, init_col_map] : init_ms)
  {
    auto new_it = new_ms.find(init_model_name);
    if(new_it == new_ms.end())
      throw std::runtime_error("Error in check for new iterator with initial model name against new_ms.");
    for(auto& [new_col, dtv_obj] : new_it->second)
    {
      std::visit([&](auto& new_raw_obj){
        using new_col_T = decltype(new_raw_obj);
        FieldAttr& new_field = db::as_ref(new_raw_obj);

        if(init_col_map.find(new_col) == init_col_map.end())
        {
          db::alter_add_column(new_it->first, new_col, new_field.sql_segment, Migrations);
          return;
        }
        std::visit([&](auto& raw_obj){
          using old_col_T = decltype(raw_obj);
          FieldAttr& init_field = db::as_ref(raw_obj);

          if(init_field.sql_segment != new_field.sql_segment)
          {
            handle_types<new_col_T, old_col_T>(new_it->first, new_col, new_field, frm, init_field, Migrations);
            if(new_field.primary_key) pk_cols.push_back(new_col);

            if(init_field.not_null != new_field.not_null)
              db::alter_column_nullable(new_it->first, new_col, !new_field.not_null, Migrations);

            if((init_field.unique != new_field.unique) && new_field.unique)
            {
              if(frm.empty())
              {
                uq_cols.push_back(new_col);
              }else{
                uq_cols.push_back(find_uq_constraint(frm, new_it->first, new_col));
              }
            }else if((init_field.unique != new_field.unique) && !new_field.unique){
              if(frm.empty())
              {
                db::drop_constraint(new_it->first, "uq_"+new_col , Migrations);
              }else{
                db::drop_constraint(new_it->first, "uq_"+find_uq_constraint(frm,new_it->first,new_col), Migrations);
              }
            }else {
              return;
            }
          }
        }, init_col_map[std::string(new_col)]);
      }, dtv_obj);

      for(const std::string& uq_col : uq_cols)
      {
        Migrations<<"ALTER TABLE " + new_it->first + " ADD ";
        db::create_uq_constraint(uq_col, Migrations);
      }
      uq_cols.clear();

      if(!mrm.empty() && !pk_cols.empty())
      {
        std::string pk_constraint = "pk_";
        for(auto& [old_mn, new_mn] : mrm.items())
        {
          if(new_mn.get<std::string>() == new_it->first)
          {
            pk_constraint += old_mn;
          }else{
            pk_constraint += new_it->first;
          }
        }
        db::drop_constraint(new_it->first, pk_constraint, Migrations);
      }else if(mrm.empty() && !pk_cols.empty()){
        db::drop_constraint(new_it->first, "pk_" + new_it->first , Migrations);
      }else{
        continue;
      }

      if(!pk_cols.empty())
      {
        Migrations<<"ALTER TABLE " + new_it->first + " ADD ";
        db::create_pk_constraint(new_it->first, pk_cols, Migrations);
        Migrations<<";\n";
      }
      pk_cols.clear();
    }
  }

  check_for_column_drops(new_ms, init_ms, Migrations);
}
