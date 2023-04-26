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

        constexpr static uint16_t SquareMask = 0x003F;
        constexpr static uint16_t NullMask   = 0x8000;

    public:
        constexpr Move() {
            Internal = 0;
        }

        template<Piece promotion = NAP, bool Null = false>
        constexpr explicit Move(const Square from, const Square to)
        {
            if (Null) Internal = NullMask;
            else      Internal = from | (to << 6) | (promotion << 12);
        }

        [[nodiscard]]
        constexpr Square From() const
        {
            return static_cast<Square>(Internal & SquareMask);
        }

        [[nodiscard]]
        constexpr Square To() const
        {
            return static_cast<Square>((Internal >> 6) & SquareMask);
        }

        [[nodiscard]]
        constexpr Piece Promotion() const
        {
            return static_cast<Piece>(Internal >> 12);
        }

        [[nodiscard]]
        constexpr bool IsNull() const
        {
            return Internal & NullMask;
        }

};

#endif //STOCKDORY_MOVE_H
