//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVE_H
#define STOCKDORY_MOVE_H

#include <cstdint>

#include "Square.h"
#include "Piece.h"

struct Move
{

    private:
        // [    PROMOTION    ] [     TO     ] [    FROM    ]
        // [     4 BITS      ] [   6 BITS   ] [   6 BITS   ]
        uint16_t Internal;

        constexpr static uint16_t SquareMask = 0x3F;

    public:
        constexpr Move() {
            Internal = 0;
        }

        template<Piece promotion = Piece::NAP>
        constexpr explicit Move(const Square from, const Square to)
        {
            Internal = from | (to << 6);
            if (promotion != Piece::NAP) Internal |= (promotion << 12);
        }

        constexpr Square From() const
        {
            return static_cast<Square>(Internal & SquareMask);
        }

        constexpr Square To() const
        {
            return static_cast<Square>((Internal >> 6) & SquareMask);
        }

        constexpr Piece Promotion() const
        {
            return static_cast<Piece>(Internal >> 12);
        }

};

#endif //STOCKDORY_MOVE_H
