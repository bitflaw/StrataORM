#pragma once
#include "db_config.hpp"

#include "variant_helpers.hpp"
#ifdef PSQL

#include "psql/alterers.hpp"
#include "psql/connectors.hpp"
#include "psql/converters.hpp"
#include "psql/create_table.hpp"
#include "psql/create_constraints.hpp"
#include "psql/deleters.hpp"
#include "psql/executor.hpp"
#include "psql/fetcher.hpp"
#include "psql/inserters.hpp"
#include "psql/queriers.hpp"
#include "psql/row_deleter.hpp"
#include "psql/updater.hpp"
#include "psql/create_model_header.hpp"
namespace db = psql;

#elif defined(MARIADB)

#include "mariadb/alterers.hpp"
#include "mariadb/connectors.hpp"
#include "mariadb/converters.hpp"
#include "mariadb/create_table.hpp"
#include "mariadb/create_constraints.hpp"
#include "mariadb/deleters.hpp"
#include "mariadb/executor.hpp"
#include "mariadb/fetcher.hpp"
#include "mariadb/inserters.hpp"
#include "mariadb/queriers.hpp"
#include "mariadb/row_deleter.hpp"
#include "mariadb/updater.hpp"
#include "mariadb/create_model_header.hpp"
namespace db = mariadb;

#else
#error "No valid db_engine specified"
#endif
