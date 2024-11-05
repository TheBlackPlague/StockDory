//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVE_H
#define STOCKDORY_MOVE_H

#include <cstdint>
#include <sstream> // Included for ToString()
#include "../Type/Square.h"
#include "../Type/Piece.h"
#include "../Util.h" // Ensure this path is correct based on your project structure

namespace StockDory
{

    struct Move
    {

    private:
        // [    PROMOTION    ] [     TO     ] [    FROM    ]
        // [     4 BITS      ] [   6 BITS   ] [   6 BITS   ]
        uint16_t Internal;

        constexpr static uint16_t SquareMask = 0x003F;

    public:
        static inline Move FromString(const std::string& str)
        {
            Square from = StockDory::Util::StringToSquare(str.substr(0, 2));
            Square to   = StockDory::Util::StringToSquare(str.substr(2, 2));

            Piece promotion = NAP;

            if (str.size() == 5) {
                switch (str[4]) {
                    case 'q':
                        promotion = Queen ;
                        break;
                    case 'r':
                        promotion = Rook  ;
                        break;
                    case 'b':
                        promotion = Bishop;
                        break;
                    case 'n':
                        promotion = Knight;
                        break;
                }
            }

            return Move(from, to, promotion);
        }

        constexpr Move() : Internal(0) // Changed to use member initializer list
        {
            // Debugging: Cannot use std::cout in constexpr context
        }

        constexpr explicit Move(const Square from, const Square to, const Piece promotion = NAP)
            : Internal(from | (to << 6) | (promotion << 12)) // Changed to use member initializer list
        {
            // Debugging: Cannot use std::cout in constexpr context
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

        [[nodiscard]]
        inline std::string ToString() const
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

} // StockDory

#endif //STOCKDORY_MOVE_H
