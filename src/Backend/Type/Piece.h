//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_PIECE_H
#define STOCKDORY_PIECE_H

#include <cstdint>
#include <string_view>

#include "../Misc.h"

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

constexpr Piece Next(const Piece p)
{
    return static_cast<Piece>(static_cast<uint8_t>(p) + 1);
}

constexpr Array<char, 7> P_CHAR {
    'P',
    'N',
    'B',
    'R',
    'Q',
    'K',
    ' '
};

constexpr char FirstLetter(const Piece p)
{
    return P_CHAR[p];
}

constexpr Array<std::string_view, 7> P_STRING {
    "Pawn",
    "Knight",
    "Bishop",
    "Rook",
    "Queen",
    "King",
    "NAP"
};

inline std::string ToString(const Piece p)
{
    return std::string(P_STRING[p]);
}

#endif //STOCKDORY_PIECE_H
