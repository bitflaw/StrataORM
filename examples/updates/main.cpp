#include <strata/db_adapters.hpp>
#include "../include/models.hpp"

int main(){
  Utils::dbenvars vars = {
    {"DBUSER", ""},
    {"DBPASS", ""},
    {"DBNAME", ""},
    {"DBHOST", ""},
    {"DBPORT", ""}
  };
  Utils::set_dbenvars(vars);

  users user {};

  db_adapter::Update<users> user_update {};
  Utils::filters filters = {
    {"username", OP::EQ, "janedoe"}
  };

  user_update.update_column("username", "email")
             .set_to("janny", "jannysimpleton@gmail.com")
             .where("and", filters)
             .commit();

  db_adapter::query::get(user, "username", "'janny'");
  return 0;
}
