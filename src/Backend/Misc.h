//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MISC_H
#define STOCKDORY_MISC_H

#include <iomanip>
#include <sstream>
#include <string>

template<typename T, typename = std::enable_if_t<std::is_fundamental_v<T>>>
std::string ToHex(const T v)
{
    std::stringstream ss;

    ss << std::setfill('0') << std::setw(sizeof(T) * 2);
    ss << std::uppercase << std::hex << v;

    return ss.str();
}

template<typename T, size_t N, size_t... Ns>
struct IArray { using Internal = std::array<typename IArray<T, Ns...>::Internal, N>; };

template<typename T, size_t N>
struct IArray<T, N> { using Internal = std::array<T, N>; };

template<typename T, size_t... Ns>
using Array = typename IArray<T, Ns...>::Internal; // Fixed size N-dimensional array of values

template<typename T, size_t N>
std::enable_if_t<!std::is_class_v<T>> Fill(std::array<T, N>& array, const T value)
{
    array.fill(value);
}

template<typename T, size_t N>
std::enable_if_t<std::is_class_v<T>> Fill(std::array<T, N>& array, const typename T::value_type value)
{
    for (auto& sub : array) Fill(sub, value);
}

#endif //STOCKDORY_MISC_H
