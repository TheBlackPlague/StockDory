//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under MIT.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef STOCKDORY_SQUARE_H
#define STOCKDORY_SQUARE_H

#include <cstdint>
#include <array>
#include <string>

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

inline constexpr Square Next(const Square sq)
{
    return static_cast<Square>(static_cast<uint8_t>(sq) + 1);
}

constexpr std::array<char, 65> FILE_CHAR {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', ' '
};

constexpr std::array<char, 65> RANK {
    '1', '1', '1', '1', '1', '1', '1', '1',
    '2', '2', '2', '2', '2', '2', '2', '2',
    '3', '3', '3', '3', '3', '3', '3', '3',
    '4', '4', '4', '4', '4', '4', '4', '4',
    '5', '5', '5', '5', '5', '5', '5', '5',
    '6', '6', '6', '6', '6', '6', '6', '6',
    '7', '7', '7', '7', '7', '7', '7', '7',
    '8', '8', '8', '8', '8', '8', '8', '8', ' '
};

inline constexpr char File(const Square sq)
{
    return FILE_CHAR[sq];
}

inline constexpr char Rank(const Square sq)
{
    return RANK[sq];
}

#endif //STOCKDORY_SQUARE_H

#pragma clang diagnostic pop