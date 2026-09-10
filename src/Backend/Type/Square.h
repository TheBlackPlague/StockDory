//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_SQUARE_H
#define STOCKDORY_SQUARE_H

#include <cstdint>
#include <string_view>

#include "../Misc.h"

enum Square : uint8_t
{

    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8, NASQ

};

constexpr Square Next(const Square sq)
{
    return static_cast<Square>(static_cast<uint8_t>(sq) + 1);
}

constexpr Array<char, 65> FILE_CHAR {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'X'
};

constexpr Array<char, 65> RANK_CHAR {
    '1', '1', '1', '1', '1', '1', '1', '1',
    '2', '2', '2', '2', '2', '2', '2', '2',
    '3', '3', '3', '3', '3', '3', '3', '3',
    '4', '4', '4', '4', '4', '4', '4', '4',
    '5', '5', '5', '5', '5', '5', '5', '5',
    '6', '6', '6', '6', '6', '6', '6', '6',
    '7', '7', '7', '7', '7', '7', '7', '7',
    '8', '8', '8', '8', '8', '8', '8', '8', '0'
};

constexpr char File(const Square sq)
{
    return FILE_CHAR[sq];
}

constexpr char Rank(const Square sq)
{
    return RANK_CHAR[sq];
}

inline std::string ToString(const Square sq)
{
    return {static_cast<char>(FILE_CHAR[sq] + ('a' - 'A')), RANK_CHAR[sq]};
}

constexpr Square FromString(const std::string_view s)
{
    if (s.size() != 2) return NASQ;

    const char file = s[0] >= 'A' && s[0] <= 'H' ? static_cast<char>(s[0] + ('a' - 'A')) : s[0];
    const char rank = s[1];

    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return NASQ;

    return static_cast<Square>((rank - '1') * 8 + file - 'a');
}

#endif //STOCKDORY_SQUARE_H
