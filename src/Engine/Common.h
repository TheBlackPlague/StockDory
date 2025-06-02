//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_COMMON_H
#define STOCKDORY_COMMON_H

#include <chrono>

#include "../Backend/Misc.h"
#include "../Backend/Type/Move.h"

namespace StockDory
{

    using Score = int32_t;

    constexpr Score Infinity = 1000000     ;
    constexpr Score Mate     = Infinity - 1;
    constexpr Score Draw     = 0           ;

    constexpr Score None     = Infinity + 1;

    constexpr uint8_t MaxDepth = 128;
    constexpr uint8_t MaxMove  = 218;

    constexpr uint16_t HistoryLimit = 16384;

    constexpr size_t MB = 1024 * 1024;

    using MS = std::chrono::milliseconds;
    using TP = std::chrono::time_point<std::chrono::steady_clock>;

    using KTable = Array<Move, 2, MaxDepth>;
    using HTable = Array<int16_t, 2, 6, 64>;

} // StockDory

#endif //STOCKDORY_COMMON_H
