//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef STOCKDORY_BITBOARD_H
#define STOCKDORY_BITBOARD_H

#include <cstdint>
#include <bit>

#include <vector>

#include "Square.h"

using BitBoard = uint64_t;

constexpr BitBoard BBDefault = 0x0000000000000000ULL;
constexpr BitBoard BBFilled  = 0xFFFFFFFFFFFFFFFFULL;

constexpr inline uint8_t Count(const BitBoard bb)
{
    return std::popcount(bb);
}

template<bool Activate>
constexpr inline void Set(BitBoard& bb, const Square sq)
{
    if (Activate) bb |=   1ULL << sq ;
    else          bb &= ~(1ULL << sq);
}

constexpr inline bool Get(const BitBoard bb, const Square sq)
{
    return bb >> sq & 1ULL;
}

constexpr inline BitBoard FromSquare(const Square sq)
{
    return 1ULL << sq;
}

constexpr inline Square ToSquare(const BitBoard bb)
{
    return static_cast<Square>(std::countr_zero(bb));
}

class BitBoardIterator
{

    private:
        BitBoard BB;

    public:
        constexpr explicit BitBoardIterator(BitBoard value)
        {
            BB = value;
        }

        constexpr inline Square Value()
        {
            uint8_t i = std::countr_zero(BB);

            // Subtract 1 and only hold set bits in the container.
            BB &= BB - 1ULL;

            return static_cast<Square>(i);
        }

        inline std::vector<Square> Values()
        {
            uint8_t count = Count(BB);
            std::vector<Square> v (count);

            for (uint8_t i = 0; i < count; i++) v[i] = Value();

            return v;
        }

};

constexpr inline BitBoardIterator Iterator(const BitBoard bb)
{
    return BitBoardIterator(bb);
}

inline std::string ToString(const BitBoard bb)
{
    std::string s;

    for (uint8_t v = 7; v != 255; v--) { // Using overflow-wrap to our advantage.
        for (uint8_t h = 0; h < 8; h++) {
            s += Get(bb, static_cast<Square>(v * 8 + h)) ? '1' : '0';
            if (h != 7) s += "  ";
        }

        s += '\n';
    }

    return s;
}

#endif //STOCKDORY_BITBOARD_H
