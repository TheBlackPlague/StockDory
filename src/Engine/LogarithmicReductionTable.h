//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_LOGARITHMICREDUCTIONTABLE_H
#define STOCKDORY_LOGARITHMICREDUCTIONTABLE_H

#include <cstdint>
#include <array>

#include "EngineParameter.h"

namespace StockDory
{

    class LogarithmicReductionTable
    {

        private:
            static std::array<std::array<int16_t, MaxMove>, MaxDepth> Internal;

        public:
            static inline int16_t Get(const uint8_t depth, const uint8_t move)
            {
                return Internal[depth][move];
            }

    };

} // StockDory

std::array<std::array<int16_t, MaxMove>, MaxDepth> StockDory::LogarithmicReductionTable::Internal = []() {
    std::array<std::array<int16_t, MaxMove>, MaxDepth> temp = {};

    for (uint8_t depth = 1; depth < MaxDepth; depth++) for (uint8_t move = 1; move < MaxMove; move++) {
        temp[depth][move] = static_cast<int16_t>(log(depth) * log(move) / 2 - 0.2);
    }

    return temp;
}();

#endif //STOCKDORY_LOGARITHMICREDUCTIONTABLE_H
