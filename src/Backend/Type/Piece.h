//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under MIT.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef STOCKDORY_PIECE_H
#define STOCKDORY_PIECE_H

#include <cstdint>

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

inline constexpr Piece Next(const Piece p)
{
    return static_cast<Piece>(static_cast<uint8_t>(p) + 1);
}

#endif //STOCKDORY_PIECE_H

#pragma clang diagnostic pop