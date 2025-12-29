#include "db_config.hpp"

#ifdef PSQL
#include "psql/datatypes.hpp"
#elif defined(MARIADB)
#include "mariadb/datatypes.hpp"
#else
#error "No database adapter specified"
#endif

void variant_to_json(nlohmann::json& j, const db::DataTypeVariant& variant);
void variant_from_json(const nlohmann::json& j, db::DataTypeVariant& variant);
