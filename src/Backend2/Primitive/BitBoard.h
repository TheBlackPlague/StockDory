//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_BITBOARD_H
#define STOCKDORY_BITBOARD_H

#include <bit>
#include <iostream>
#include <iomanip>

#include "Base.h"
#include "Square.h"

namespace StockDory
{

    using BitBoard = u64;

    constexpr u08 Count(const BitBoard bb) { return std::popcount(bb); }

    template<bool Activate>
    constexpr void Set(BitBoard& bb, const Square sq)
    {
        if (Activate) bb |=   1ULL << sq ;
        else          bb &= ~(1ULL << sq);
    }

    constexpr bool Get(const BitBoard bb, const Square sq) { return bb >> sq & 1ULL; }

    constexpr BitBoard FromSquare(const Square sq) { return 1ULL << sq; }

    constexpr Square ToSquare(const BitBoard bb) { return static_cast<Square>(std::countr_zero(bb)); }

    class BitBoardIterator
    {

        BitBoard Internal;

        public:
        constexpr BitBoardIterator(const BitBoard bb) : Internal(bb) {}

        class Iterator
        {

            BitBoard Internal;

            public:
            constexpr Iterator(const BitBoard bb) : Internal(bb) {}

            constexpr Square operator *() const { return ToSquare(Internal); }

            constexpr Iterator& operator ++() { Internal &= Internal - 1ULL; return *this; }

            constexpr bool operator !=(const Iterator& other) const { return Internal != other.Internal; }

        };

        constexpr Iterator begin() const { return Iterator(Internal); }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        constexpr Iterator end()   const { return Iterator(0); }

    };

    OutputStream& operator <<(OutputStream& os, const BitBoard bb)
    {
        for (u08 rank = 7; rank < 8; rank--)
        for (u08 file = 0; file < 8; file++) {
            const auto sq = static_cast<Square>(rank * 8 + file);

            os << (Get(bb, sq) ? "1" : "0");
        }

        os << '\n' << AsHex(bb) << '\n';

        return os;
    }

    struct Pin   { BitBoard StraightMask = 0, DiagonalMask = 0; };

    struct Check { BitBoard Mask = 0; bool Double = false; };

} // StockDory

#endif //STOCKDORY_BITBOARD_H
