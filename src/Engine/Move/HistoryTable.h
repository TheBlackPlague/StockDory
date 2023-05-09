//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_HISTORYTABLE_H
#define STOCKDORY_HISTORYTABLE_H

#include <cstdint>
#include <array>

#include "../../Backend/Type/Move.h"
#include "../../Backend/Type/Color.h"

namespace StockDory
{

    class HistoryTable
    {

        private:
            std::array<std::array<std::array<int32_t, 64>, 6>, 2> Internal = {};

        public:
            [[nodiscard]]
            inline int32_t Get(const Piece piece, const Color color, const Square sq) const
            {
                return Internal[color][piece][sq];
            }

            [[nodiscard]]
            inline int32_t& Get(const Piece piece, const Color color, const Square sq)
            {
                return Internal[color][piece][sq];
            }

    };

} // StockDory

#endif //STOCKDORY_HISTORYTABLE_H
