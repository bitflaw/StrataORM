#include "../models.hpp"
#include "../db_config.hpp"

#ifdef PSQL
namespace psql {
void create_models_hpp(const ms_map& migrations);
}
namespace db_adapter = psql;
#endif
