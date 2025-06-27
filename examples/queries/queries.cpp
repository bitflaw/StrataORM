#include <iostream>
#include <strata/db_adapters.hpp>
#include "../include/models.hpp"

int main(){
  users user {};
  message m {};

  db_adapter::query::fetch_all(user, "*");
  //db_adapter::query::get(user, "username", "berna");
  /*filters filters = {
    {"email", OP::CONTAINS, "gmail"},
    {"username", OP::STARTSWITH, "b"}
  };

  db_adapter::query::filter(user, "or", filters);*/

  int records_size = user.records.size();
  std::vector<users> my_users = db_adapter::query::to_instances(user);

  for (int i = 0; i < records_size; ++i) {
    std::cout<< my_users[i].id<<": "
             << my_users[i].username<<", "
             << my_users[i].email<<", "
             << my_users[i].pin<< "\n";
  }
  std::cout<<std::endl;

  /*db_adapter::query::JoinBuilder JB {user};
  pqxx::result result = JB.select("username, email")
                          .inner_join("message")
                          .on("and", "users.users_id = message.sender")
                          .execute();

  std::cout<<"results returned: "<<result.size()<<std::endl;*/
  return 0;
}
