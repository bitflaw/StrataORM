#pragma once
#include "../db_config.hpp"

#ifdef MARIADB

#include <mdbcxx/row.hpp>
namespace mariadb
{

template <typename Model_T>
Model_T to_instance(mcxx::Row& row)
{
  using tuple_T = decltype(std::declval<Model_T>().get_attr());
  return Model_T(row.template as_tuple<tuple_T>());
}

template <typename Model_T>
std::vector<Model_T> to_instances(Model_T& obj)
{
  using tuple_T = decltype(obj.get_attr());
  std::vector<Model_T> instances {};
  instances.reserve(obj.records.size());

  for(const mcxx::Row& row : obj.records)
    instances.push_back(Model_T(row.template as_tuple<tuple_T>()));
  return instances;
}

template <typename Model_T>
decltype(std::declval<Model_T>().get_attr()) to_tuple(mcxx::Row& row)
{
  using tuple_T = decltype(std::declval<Model_T>().get_attr());
  return row.template as_tuple<tuple_T>();
}

template <typename Model_T>
std::vector<decltype(std::declval<Model_T>().get_attr())> to_tuples(Model_T& obj)
{
  using tuple_T = decltype(obj.get_attr());
  std::vector<tuple_T> values {};
  values.reserve(obj.records.size());

  for(const mcxx::Row& row : obj.records)
    values.push_back(row.template as_tuple<tuple_T>());
  return values;
}

}
namespace db = mariadb;
#endif
