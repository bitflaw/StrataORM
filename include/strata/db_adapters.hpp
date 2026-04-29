#pragma once
#include <strata/db_config.hpp>

#include <strata/variant_helpers.hpp>
#ifdef PSQL

#include <strata/psql/alterers.hpp>
#include <strata/psql/connectors.hpp>
#include <strata/psql/converters.hpp>
#include <strata/psql/create_table.hpp>
#include <strata/psql/create_constraints.hpp>
#include <strata/psql/deleters.hpp>
#include <strata/psql/executor.hpp>
#include <strata/psql/fetcher.hpp>
#include <strata/psql/inserters.hpp>
#include <strata/psql/queriers.hpp>
#include <strata/psql/row_deleter.hpp>
#include <strata/psql/updater.hpp>
#include <strata/psql/create_model_header.hpp>
namespace db = psql;

#elif defined(MARIADB)

#include <strata/mariadb/alterers.hpp>
#include <strata/mariadb/connectors.hpp>
#include <strata/mariadb/converters.hpp>
#include <strata/mariadb/create_table.hpp>
#include <strata/mariadb/create_constraints.hpp>
#include <strata/mariadb/deleters.hpp>
#include <strata/mariadb/executor.hpp>
#include <strata/mariadb/fetcher.hpp>
#include <strata/mariadb/inserters.hpp>
#include <strata/mariadb/queriers.hpp>
#include <strata/mariadb/row_deleter.hpp>
#include <strata/mariadb/updater.hpp>
#include <strata/mariadb/create_model_header.hpp>
namespace db = mariadb;

#else
#error "No valid db_engine specified"
#endif
