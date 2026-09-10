//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_MISC_H
#define STOCKDORY_MISC_H

#include <array>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

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

template<typename T, size_t N, typename V>
constexpr void Fill(std::array<T, N>& array, const V& value)
{
    if constexpr (requires { array.fill(value); }) array.fill(value);
    else
        for (auto& sub : array) Fill(sub, value);
}

#endif //STOCKDORY_MISC_H
