//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVE_H
#define STOCKDORY_MOVE_H

#include <cstdint>

#include "Square.h"
#include "Piece.h"
#include "../Util.h"

struct Move
{

    private:
        // [    PROMOTION    ] [     TO     ] [    FROM    ]
        // [     4 BITS      ] [   6 BITS   ] [   6 BITS   ]
        uint16_t Internal;

        constexpr static uint16_t SquareMask = 0x003F;

    public:
        constexpr Move()
        {
            Internal = 0;
        }

        constexpr explicit Move(const Square from, const Square to, const Piece promotion = NAP)
        {
            Internal = from | (to << 6) | (promotion << 12);
        }

        [[nodiscard]]
        constexpr inline Square From() const
        {
            return static_cast<Square>(Internal & SquareMask);
        }

        [[nodiscard]]
        constexpr inline Square To() const
        {
            return static_cast<Square>((Internal >> 6) & SquareMask);
        }

        [[nodiscard]]
        constexpr inline Piece Promotion() const
        {
            return static_cast<Piece>(Internal >> 12);
        }

        [[nodiscard]]
        constexpr inline bool operator==(const Move other) const
        {
            return Internal == other.Internal;
        }

        std::string ToString() const
        {
            std::stringstream s;

            Square from      = From     ();
            Square to        = To       ();
            Piece  promotion = Promotion();

            s << StockDory::Util::SquareToString(from) << StockDory::Util::SquareToString(to);

            if (promotion != NAP)
                s << static_cast<char>(tolower(FirstLetter(promotion)));

            return s.str();
        }

};

#endif //STOCKDORY_MOVE_H
