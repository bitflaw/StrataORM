#pragma once
#include <array>
#include <stdexcept>
#include <string>
#include "concepts.hpp"

namespace Utils {

template <typename T, std::size_t N>
struct CustomArray{
  std::array<T, N> wrapped_array {};
  std::size_t index = 0;

  constexpr CustomArray() = default;

  template<typename... Args>
  requires (sizeof...(Args) <= N) && (all_convertible_to_T<std::string, Args...> || all_same_as_T<std::string, Args...>)
  constexpr CustomArray(Args&&... args): wrapped_array {static_cast<T>(args)...}, index(sizeof...(args)){}

  constexpr void push_back(T value){
    if(index >= N) throw std::length_error("Custom Array is up to capacity!");
    wrapped_array[index++] = value;
  }

  constexpr auto begin(){ return wrapped_array.begin(); }
  constexpr auto begin() const { return wrapped_array.begin(); }
  constexpr auto end(){ return wrapped_array.begin() + index; }
  constexpr auto end() const { return wrapped_array.begin() + index; }

  constexpr T& operator[](std::size_t i){
    if (i >= index) throw "CustomArray: Index out of bounds!";
    return wrapped_array[i];
  }
  constexpr const T& operator[](std::size_t i) const {
    if (i >= index) throw "CustomArray: Index out of bounds!";
    return wrapped_array[i];
  }

  constexpr std::size_t size() const { return index; }
  constexpr std::size_t max_size() const { return N; }
};

}
