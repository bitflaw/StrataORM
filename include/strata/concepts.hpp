#pragma once
#include <concepts>

template<typename T, typename... Args>
concept all_convertible_to_T = (std::convertible_to<Args, T> && ...);

template<typename T, typename... Args>
concept all_same_as_T = (std::same_as<T, Args> && ...);
