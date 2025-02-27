#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "./datatypes.hpp"

using ms_map = std::unordered_map<std::string, std::unordered_map<std::string, DataTypeVariant>>;

class Model{
public:
  std::unordered_map<std::string, DataTypeVariant> fields;
  ms_map init_ms;
  ms_map new_ms;

  Model(){
    fields.clear();
  };
  void make_migrations();
  void track_changes();
};

class ModelFactory{
public:
  using Creator = std::function<std::unique_ptr<Model>()>;

  static std::unordered_map<std::string, Creator>& registry(){
    static std::unordered_map<std::string, Creator> registry_map;
    return registry_map;
  }

  static void register_model(const std::string& model_name, Creator creator){
    registry()[model_name] = std::move(creator);
  }
  static std::unique_ptr<Model> create_model_instance(const std::string& model_name){
    auto it = registry().find(model_name);
    if(it != registry().end()){
      return it->second();
    }
    return nullptr;
  }
};

#define REGISTER_MODEL(MODEL)\
static bool registered_##MODEL = [](){\
                                    ModelFactory::register_model(#MODEL, []() -> std::unique_ptr<Model>{\
                                      return std::make_unique<MODEL>();\
                                    });\
                                    return true;\
                                  }()

