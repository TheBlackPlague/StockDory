//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef STOCKDORY_PIECE_H
#define STOCKDORY_PIECE_H

#include <cstdint>
#include <map>

enum Piece : uint8_t
{

    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    NAP

};

constexpr inline Piece Next(const Piece p)
{
    return static_cast<Piece>(static_cast<uint8_t>(p) + 1);
}

constexpr std::array P_CHAR = {
    'P',
    'N',
    'B',
    'R',
    'Q',
    'K',
    ' '
};

constexpr inline char FirstLetter(const Piece p)
{
    return P_CHAR[p];
}

inline std::map<Piece, std::string> P_STRING = {
    {Pawn  , "Pawn"  },
    {Knight, "Knight"},
    {Bishop, "Bishop"},
    {Rook  , "Rook"  },
    {Queen , "Queen" },
    {King  , "King"  },
    {NAP   , "NAP"   }
};

inline std::string ToString(const Piece p)
{
    return P_STRING[p];
}

#endif //STOCKDORY_PIECE_H
