//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SQUARE_H
#define STOCKDORY_SQUARE_H

#include "Base.h"

namespace StockDory
{

    enum Square : u08
    {

        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8, InvalidSquare

    };

    constexpr Square operator ++(Square& sq) { sq = static_cast<Square>(sq + 1); return sq; }

    constexpr Square operator ++(Square& sq, int)
    {
        const Square temp = sq;

        sq = static_cast<Square>(sq + 1);

        return temp;
    }

    InputStream& operator >>(InputStream& is, Square& sq)
    {
        char file, rank;

        is >> file >> rank;

        file = tolower(file);
        rank = tolower(rank);

        if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
            sq = InvalidSquare;
            return is;
        }

        sq = static_cast<Square>((rank - '1') * 8 + (file - 'a'));

        return is;
    }

    OutputStream& operator <<(OutputStream& os, const Square sq)
    {
        constexpr static Array<char, 8> File { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
        constexpr static Array<char, 8> Rank { '1', '2', '3', '4', '5', '6', '7', '8' };

        if (sq == InvalidSquare) return os << "xx";

        return os << File[sq % 8] << Rank[sq / 8];
    }

} // StockDory

#endif //STOCKDORY_SQUARE_H
