//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_LOGARITHMICREDUCTIONTABLE_H
#define STOCKDORY_LOGARITHMICREDUCTIONTABLE_H

#include <array>
#include <cmath>
#include <cstdint>

#include "EngineParameter.h"

namespace StockDory
{

    std::array<std::array<int16_t, MaxMove>, MaxDepth> LogarithmicReductionTable =
    [] -> std::array<std::array<int16_t, MaxMove>, MaxDepth>
    {
        std::array<std::array<int16_t, MaxMove>, MaxDepth> temp = {};

        for (uint8_t depth  = 1; depth < MaxDepth; depth++)
        for (uint8_t move   = 1; move  < MaxMove ;  move++)
            temp[depth]
                [move ] = static_cast<int16_t>(std::log(depth) * std::log(move) / 2 - 0.2);

        return temp;
    }();

} // StockDory

#endif //STOCKDORY_LOGARITHMICREDUCTIONTABLE_H
