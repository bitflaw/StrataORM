#include "../include/models.hpp"
#include <strata/db_adapters.hpp>

int main(){
  users user {};
  message m {};

  using params = std::vector<pqxx::params>;
  params user_rows = user.parse_json_rows();
  params message_rows = m.parse_json_rows();

  pqxx::connection cxn = db_adapter::prepare_insert<users>();
  for(pqxx::params& user_row : user_rows){
    db_adapter::exec_insert(cxn, user_row);
  }

  cxn = db_adapter::prepare_insert<message>();
  for(pqxx::params& message_r : message_rows){
    db_adapter::exec_insert(cxn, message_r);
  }

  return 0;
}
