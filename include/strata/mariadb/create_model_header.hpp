#include <strata/db_config.hpp>

#ifdef MARIADB

#include <strata/models.hpp>

namespace mariadb
{
void create_models_hpp(const ms_map& migrations);
}
namespace db = mariadb;

#endif
