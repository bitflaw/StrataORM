#include "json.hpp"
#include <string>

class FieldAttr {
public:
	std::string ctype, datatype, sql_segment;
	bool primary_key, not_null, unique;

	FieldAttr(std::string ctype = "null", std::string datatype = "null", bool not_null = false, bool unique = false, bool pk = false)
	: ctype(ctype), datatype(datatype), primary_key(pk), not_null(not_null), unique(unique)
	{}

  virtual void gen_sql() = 0;
  virtual void from_json(const nlohmann::json&) = 0;
  virtual void to_json(nlohmann::json&) const = 0;
  virtual void track(std::string new_model_name, const std::string col_name, FieldAttr& old_col_obj,
                     std::ofstream& Migrations, const nlohmann::json& frm = {}) const = 0;

  virtual ~FieldAttr() = default;
};
