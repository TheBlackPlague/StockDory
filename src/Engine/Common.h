//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_COMMON_H
#define STOCKDORY_COMMON_H

#include <cstdint>

#define BEGIN_PACK __pragma(pack(push, 1))
#define END_PACK   __pragma(pack(pop))

namespace StockDory
{

    using Score = int16_t;

    constexpr Score Infinity = 32000       ;
    constexpr Score Mate     = Infinity - 1;
    constexpr Score Draw     = 0           ;

    constexpr uint8_t MaxDepth = 128;
    constexpr uint8_t MaxMove  = 218;

    constexpr size_t MB = 1024 * 1024;

} // StockDory

#endif //STOCKDORY_COMMON_H
