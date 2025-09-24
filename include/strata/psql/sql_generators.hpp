#pragma once
#include "../db_config.hpp"
#include "../datatypes.hpp"

#ifdef PSQL
namespace psql{
void generate_int_sql(IntegerField& int_obj);

void generate_char_sql(CharField& char_obj);

void generate_decimal_sql(DecimalField& dec_obj);

void generate_bool_sql(BoolField& bool_obj);

void generate_bin_sql(BinaryField& bin_obj);

void generate_datetime_sql(DateTimeField& dt_obj);

void generate_foreignkey_sql(ForeignKey& fk_obj);

}

namespace db_adapter = psql;
#endif
