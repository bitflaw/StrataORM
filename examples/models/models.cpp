#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <strata/models.hpp>
#include <strata/db_adapters.hpp>

class users : public Model{
public:
  users(){
    col_map["username"] = std::make_shared<CharField>(CharField("varchar", 24, true, true));
    col_map["email"] = std::make_shared<CharField>(CharField("varchar", 50, true, true));
    col_map["pin"] = std::make_shared<IntegerField>(IntegerField("integer", false, true));
  }
};REGISTER_MODEL(users);

class message : public Model{
public:
  message(){
    col_map["sender"] = std::make_shared<ForeignKey>(ForeignKey("sender", "users", "users_id", std::nullopt, "CASCADE", "CASCADE"));
    col_map["receiver"] = std::make_shared<ForeignKey>(ForeignKey("receiver", "users", "users_id", std::nullopt, "CASCADE", "CASCADE"));
    col_map["content"] = std::make_shared<CharField>(CharField("varchar", 256, true));
  }
};REGISTER_MODEL(message);

int main(){
  Model model {};
  nlohmann::json mrm {};
  nlohmann::json frm {};
  std::string sql_filename {"migrations.sql"};

  model.make_migrations(mrm, frm, sql_filename);

  std::optional<pqxx::result> result = db_adapter::execute_sql(sql_filename);

  return 0;
}
