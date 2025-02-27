#pragma once
#include <vector>

namespace query{

  template <typename Model_T>
  void fetch_all(Model_T& obj);

  template <typename Model_T, typename... Args>
  void get(Model_T& obj, Args... args);

  template <typename Model_T, typename... Args>
  void filter(Model_T& obj, Args... args);

  template <typename Model_T, typename... Args>
  void select_related(Model_T& obj, Args... args);

  template <typename Model_T>
  std::vector<Model_T> to_instances(Model_T& obj);

  template <typename Model_T>
  std::vector<decltype(std::declval<Model_T>.get_attr())> to_values(Model_T& obj);
}
