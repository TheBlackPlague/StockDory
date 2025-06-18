//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_BASE_H
#define STOCKDORY_BASE_H

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

namespace StockDory
{

    using u08 = uint8_t ;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using i08 =  int8_t ;
    using i16 =  int16_t;
    using i32 =  int32_t;
    using i64 =  int64_t;

    using f32 =  float;
    using f64 = double;

    using usize = size_t;

    using TID = usize;

    using Score = i32;

    template<typename T, usize N, usize... Ns>
    struct IArray { using Base = std::array<typename IArray<T, Ns...>::Base, N>; };

    template<typename T, usize N>
    struct IArray<T, N> { using Base = std::array<T, N>; };

    template<typename T, usize... Ns>
    using Array = typename IArray<T, Ns...>::Base; // Fixed-size N-dimensional array of type T

    template<typename T>
    using VarArray = std::vector<T>; // Variable-size heap-allocated array of type T

    using OutputStream = std::ostream; // Output stream type

    using InputStream = std::istream; // Input stream type

    using String = std::string; // String type

    using StringStream = std::stringstream; // String stream type

    using OutputStringStream = std::ostringstream; // Output string stream type

    using InputStringStream = std::istringstream; // Input string stream type

    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    String AsHex(const T v)
    {
        std::stringstream ss;

        ss << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << v;

        return ss.str();
    }

} // StockDory

#endif //STOCKDORY_BASE_H
