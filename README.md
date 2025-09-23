# StrataORM
An ORM built on C++20 that is inspired by Django's ORM, built to support multiple database engines.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20.html)
[![CMake](https://img.shields.io/badge/Build-CMake%203.16%2B-brightblue)](https://cmake.org)
[![PostgreSQL](https://img.shields.io/badge/Database-PostgreSQL-blueviolet)](https://www.postgresql.org)
[![libpqxx](https://img.shields.io/badge/Dependency-libpqxx--dev-pink)](https://github.com/jtv/libpqxx)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-darkred)](https://www.gnu.org/licenses/gpl-3.0.en.html)

Strata provides an easy-to-use, intuitive API to interact with databases. As of now, it only
supports PostgreSQL but plans to add support for more database engines are in mind.

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
- [ ] Support for nullable values.
- [ ] Support for user-defined datatypes.
- [ ] Support for more database engines eg MySQL, SQLite, MSSQL etc.

## Dependencies
Since this library supports only PostgreSQL now, dependencies are:
- A C++ compiler supporting [```-std=C++20```](https://en.cppreference.com/w/cpp/compiler_support/20).
- [CMake](https://cmake.org/download/) (at least version 3.16).
- [PostgreSQL](https://www.postgresql.org/download/) library.
- [libpqxx](https://github.com/jtv/libpqxx) -> Official C++ client library for postgres.


>[!Warning]
> This library depends on a feature from libpqxx that is only available on the latest development version(not released yet).
> Options are to build from source or wait for the upcoming [```8.0```](https://github.com/jtv/libpqxx/pull/914) release.


## Build & Installation

### Step 1: Clone the repository
```bash
$ git clone git@github.com:bitflaw/strataorm.git
$ cd strataorm
```
### Step 2: Build the library
Since we are using CMake, I recommend building in a dedicated build directory:
```bash
$ mkdir build
$ cmake -B ${BUILD_DIR} -S . -DDB_ENGINE=PSQL
$ cmake --build ${BUILD_DIR}
```

- ```-DDB_ENGINE=PSQL``` to specify the database you want to use the ORM with.
  This flag only takes ```PSQL``` for now since only postgres is supported for now.
- Note that both static and dynamic libraries will be built for both use cases, avoiding rebuilding just to
use a desired one.


### Step 3: Install to System
To install to the default location specified by CMake, run:
```bash
$ cmake --install ${BUILD_DIR}
```
To install to a specified location, do:
```bash
$ cmake --install ${BUILD_DIR} --prefix ${DESTINATION}
```
> [!NOTE]
> You might need sudo/admin privileges to run this command.


If it's a CMake project, add this in your CMakeLists.txt file immediately after cloning the repo:
```
add_subdirectory(strata)
target_link_libraries(my_project PRIVATE strata)
```

## Examples
Examples can be found under the ```examples``` directory in the source tree.

**Model usage example**
```cpp
#include <memory>
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
  Utils::dbenvars vars = {
    //insert your db credentials here
    {"DBUSER", ""},
    {"DBPASS", ""},
    {"DBNAME", ""},
    {"DBHOST", ""},
    {"DBPORT", ""}
  };
  Utils::set_dbenvars(vars);

  Model model {};
  nlohmann::json mrm {};
  nlohmann::json frm {};
  std::string sql_filename {"migrations.sql"};

  model.make_migrations(mrm, frm, sql_filename);

  db_adapter::opt_result_t result = db_adapter::execute_sql(sql_filename);
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

  db_adapter::query::fetch_all(user, "*");
  //db_adapter::query::get(user, "username", "berna");
  filters filters = {
    {"email", OP::CONTAINS, "gmail"},
    {"username", OP::STARTSWITH, "b"}
  };

  db_adapter::query::filter(user, "or", filters);

  std::vector<users> my_users = db_adapter::query::to_instances(user);

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
  db_adapter::query::JoinBuilder JB {user};
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
  db_adapter::delete_row<users>("and", filters);

  return 0;
}
```

> [!NOTE]
> Tests have not been implemented yet but will be soon.

## Contributing
All contributions are welcome. Please open an issue or submit a pull request for contributions to the library.
