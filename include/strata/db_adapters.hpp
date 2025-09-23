#pragma once
#include "db_config.hpp"

#ifdef PSQL

#include "psql/alterers.hpp"
#include "psql/connectors.hpp"
#include "psql/converters.hpp"
#include "psql/creators.hpp"
#include "psql/deleters.hpp"
#include "psql/executor.hpp"
#include "psql/fetcher.hpp"
#include "psql/inserters.hpp"
#include "psql/queriers.hpp"
#include "psql/row_deleter.hpp"
#include "psql/updater.hpp"
#include "psql/sql_generators.hpp"
#include "psql/create_model_header.hpp"
namespace db_adapter = psql;

#else
#error "No valid db_engine specified"
#endif
