//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVE_H
#define STOCKDORY_MOVE_H

#include <cstdint>

#include "Piece.h"
#include "Square.h"

struct Move
{

    private:
    constexpr static uint16_t   SquareMask = 0x003F;
    constexpr static uint8_t         ToPos =      6;
    constexpr static uint8_t  PromotionPos =     12;

    // [    PROMOTION    ] [     TO     ] [    FROM    ]
    // [     4 BITS      ] [   6 BITS   ] [   6 BITS   ]
    uint16_t Internal;

    public:
    static Move FromString(const std::string& str)
    {
        const Square from = ::FromString(str.substr(0, 2));
        const Square to   = ::FromString(str.substr(2, 2));

        Piece promotion = NAP;

        if (str.size() == 5) {
            switch (str[4]) {
                case 'q':
                    promotion = Queen;
                    break;
                case 'r':
                    promotion = Rook;
                    break;
                case 'b':
                    promotion = Bishop;
                    break;
                case 'n':
                    promotion = Knight;
                    break;
                default: ;
            }
        }

        return Move(from, to, promotion);
    }

    constexpr Move()
    {
        Internal = 0;
    }

    constexpr Move(const Square from, const Square to, const Piece promotion = NAP)
    {
        Internal = from | to << ToPos | promotion << PromotionPos;
    }

    [[nodiscard]]
    constexpr Square From() const
    {
        return static_cast<Square>(Internal & SquareMask);
    }

    [[nodiscard]]
    constexpr Square To() const
    {
        return static_cast<Square>(Internal >> ToPos & SquareMask);
    }

    [[nodiscard]]
    constexpr Piece Promotion() const
    {
        return static_cast<Piece>(Internal >> PromotionPos);
    }

    [[nodiscard]]
    constexpr bool operator==(const Move other) const
    {
        return Internal == other.Internal;
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator bool() const { return Internal != 0; }

    [[nodiscard]]
    std::string ToString() const
    {
        std::stringstream s;

        const Square from      =      From();
        const Square to        =        To();
        const Piece  promotion = Promotion();

        s << ::ToString(from) << ::ToString(to);

        if (promotion != NAP) s << static_cast<char>(tolower(FirstLetter(promotion)));

        return s.str();
    }

};

#endif //STOCKDORY_MOVE_H
