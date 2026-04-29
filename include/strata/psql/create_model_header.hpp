# pragma once
#include <strata/db_config.hpp>

#ifdef PSQL

#include <strata/models.hpp>

namespace psql {
void create_models_hpp(const ms_map& migrations);
}
namespace db = psql;
#endif
