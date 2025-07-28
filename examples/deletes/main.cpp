#include "../include/models.hpp"
#include <strata/db_adapters.hpp>

int main(){
  users user {};

  db_adapter::query::fetch_all(user, "*");
  int records_size = user.records.size();
  std::vector<users> my_users = db_adapter::to_instances(user);
  std::cout<< "Before deleting a user with id 3:\n";
  for (int i = 0; i < records_size; ++i) {
    std::cout<< my_users[i].id<<": "
             << my_users[i].username<<", "
             << my_users[i].email<<", "
             << my_users[i].pin<< "\n";
  }
  std::cout<<std::endl;

  Utils::filters filters = {
    {"users_id", OP::EQ, 3}
  };
  db_adapter::delete_row<users>("and", filters);

  user.records.clear();
  db_adapter::query::fetch_all(user, "*");
  records_size = user.records.size();
  my_users = db_adapter::to_instances(user);
  std::cout<< "After deleting user with id 3:\n";
  for (int i = 0; i < records_size; ++i) {
    std::cout<< my_users[i].id<<": "
             << my_users[i].username<<", "
             << my_users[i].email<<", "
             << my_users[i].pin<< "\n";
  }
  std::cout<<std::endl;

  return 0;
}
