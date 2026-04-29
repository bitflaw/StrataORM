#include <strata/db_config.hpp>
#include <type_traits>

#ifdef PSQL
#include "../include/strata/psql/datatypes.hpp"
#elif defined(MARIADB)
#include "../include/strata/mariadb/datatypes.hpp"
#else
#error "No database adapter specified"
#endif

template <typename T>
bool try_set_variant(const nlohmann::json& j, db::DataTypeVariant& variant) {
  try {
    T value {};
    if constexpr (std::is_same_v<T, std::shared_ptr<FieldAttr>>) value->from_json(j);
    else value.from_json(j);
    variant = value;
    return true;
  }catch (...) {
    return false;
  }
}

template <typename... Ts>
bool try_deserialize(const nlohmann::json& j, db::DataTypeVariant& variant, std::variant<Ts...>*) {
  return ((try_set_variant<Ts>(j, variant)) || ...);
}

void variant_to_json(nlohmann::json& j, const db::DataTypeVariant& variant){
  std::visit([&j](auto& obj) mutable {
    using T = std::decay_t<decltype(obj)>;
    if constexpr (std::is_same_v<T, std::shared_ptr<FieldAttr>>) obj->to_json(j);
    else obj.to_json(j);
  }, variant);
}

void variant_from_json(const nlohmann::json& j, db::DataTypeVariant& variant) {
  if (!try_deserialize(j, variant, static_cast<db::DataTypeVariant*>(nullptr))) {
    throw std::invalid_argument("Error occured while parsing JSON back to objects.");
  }
}
