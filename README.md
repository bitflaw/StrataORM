# StrataORM
An ORM built on C++20 that is inspired by Django's ORM, built to support multiple database engines.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20.html)
[![CMake](https://img.shields.io/badge/Build-CMake%203.16%2B-brightblue)](https://cmake.org)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-darkred)](https://www.gnu.org/licenses/gpl-3.0.en.html)

Strata provides an easy-to-use, intuitive API to interact with databases.
Supports the following databases:
    - PostgreSQL
    - MariaDB

Documentation can be found at the [WIKI](https://github.com/bitflaw/strataorm/wiki).

>[!Warning]
>This library is still under ACTIVE development and should not be considered stable!

## Features
- [X] CRUD operations for rows of data.
- [X] Class-based models representing SQL tables.
- [X] Migration tracking between changes in models and their columns.
- [X] Support for raw SQL execution.
- [X] Clean abstraction over raw, basic SQL datatypes using classes.
- [X] Support for performing fetches, filters(limited) and joins.
- [X] Environmental variables set in-program for db connections.
- [X] Support for user-defined datatypes.
- [X] Support for more database engines ie. postgres and Mariadb
- [ ] Support for nullable values.

## Dependencies
- A C++ compiler supporting [```-std=C++20```](https://en.cppreference.com/w/cpp/compiler_support/20).
- [CMake](https://cmake.org/download/) (at least version 3.16).
**If using PostgreSQL**
- [libpqxx](https://github.com/jtv/libpqxx) -> Official C++ client library for postgres.
- [PostgreSQL](https://postgresql.org/download) -> PostgreSQL database.
**If using MariaDB**
- [mdbcxx](https://github.com/bitflaw/mdbcxx) -> A MariaDB C++ Connector.
- [MariaDB's C connector](https://github.com/mariadb-corporation/mariadb-connector-c) -> C connector for MariaDB which is a dependency for `mdbcxx`
- [MariaDB](https://mariadb.com/downloads) -> MariaDB database

## Build & Installation
```bash
git clone git@github.com:bitflaw/strataorm.git
cd strataorm
cmake -B ${BUILD_DIR} -S . -DDB_ENGINE={PSQL or MARIADB}
cmake --build ${BUILD_DIR}
cmake --install ${BUILD_DIR} #installs to /usr/local/
```

**Model usage example**
```cpp
#include <strata/models.hpp>
#include <strata/db_adapters.hpp>

class users : public Model{
public:
  users(){
    col_map["username"] = db::Field::CharField("varchar", 24, true, false);
    col_map["email"] = db::Field::CharField("varchar", 51, false, true);
    col_map["pin"] = db::Field::IntegerField("tinyint", false, true);
  }
};REGISTER_MODEL(users);

int main(){
  //can remove if u don't plan to apply the changes to the actual db.
  //this is only relevant when there is a db in play.
  // Utils::dbenvars vars = {
  //   {"DBUSER", ""},
  //   {"DBPASS", ""},
  //   {"DBNAME", ""},
  //   {"DBHOST", ""},
  //   {"DBPORT", ""}
  // };
  // Utils::set_dbenvars(vars);

  Model model {};
  nlohmann::json mrm {};
  nlohmann::json frm {};
  std::string sql_filename {"migrations.sql"};

  model.make_migrations(mrm, frm, sql_filename);
  // db::opt_result_t result = db::execute_sql(sql_filename);
  return 0;
}
```

**Usage of inserts**
This example uses a user-defined function ```.parse_json_rows()``` defined inside the corresponding classes to convert json objects into ```pqxx::params```  which
```exec_insert``` needs to insert into the database.
```cpp
#include "./include/models.hpp"
#include <strata/db_adapters.hpp>

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

  using params = std::vector<pqxx::params>;
  params user_rows = user.parse_json_rows();

  pqxx::connection cxn = db::prepare_insert<users>();
  for(pqxx::params& user_row : user_rows){
    db::exec_insert(cxn, user_row);
  }
  return 0;
}
```

**Queries Example**
```cpp
#include <strata/db_adapters.hpp>
#include "./include/models.hpp"

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

  db::query::fetch_all(user, "*");
  //db::query::get(user, "username", "berna");
  filters filters = {
    {"email", OP::CONTAINS, "gmail"},
    {"username", OP::STARTSWITH, "b"}
  };

  db::query::filter(user, "or", filters);

  std::vector<users> my_users = db::query::to_instances(user);

  return 0;
}
```

**Joins Example**
```cpp
#include <strata/db_adapters.hpp>
#include "./include/models.hpp"

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
  db::query::JoinBuilder JB {user};
  pqxx::result result = JB.select("username, email")
                          .inner_join("message")
                          .on("and", "users.users_id = message.sender")
                          .execute();

  std::cout<<"Number of results returned: "<<result.size()<<std::endl;
  return 0;
}
```

**Updates Example**
```cpp
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

  db::Update<users> user_update {};
  Utils::filters filters = {
    {"username", OP::EQ, "janedoe"}
  };

  user_update.update_column("username", "email")
             .set_to("janny", "jannysimpleton@gmail.com")
             .where("and", filters)
             .commit();

  db::query::get(user, "username", "'janny'");
  return 0;
}
```

**Delete Example**
```cpp
#include "../include/models.hpp"
#include <strata/db_adapters.hpp>

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
  Utils::filters filters = {
    {"users_id", OP::EQ, 3}
  };
  db::delete_row<users>("and", filters);

  return 0;
}
```

> [!NOTE]
> Tests have not been implemented yet but will be soon.

