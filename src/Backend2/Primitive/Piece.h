//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PIECE_H
#define STOCKDORY_PIECE_H

#include "PieceType.h"
#include "Side.h"

namespace StockDory
{

    class Piece
    {

        constexpr static u08 PieceTypeMask = 0b111;
        constexpr static u08 SideShift     =     3;

        u08 Internal;

        public:
        constexpr Piece(const Side side, const PieceType type) { Internal = type | side << SideShift; }

        constexpr PieceType Type() const { return static_cast<PieceType>(Internal & PieceTypeMask); }

        constexpr Side Side() const { return static_cast<enum Side>(Internal >> SideShift); }

    };

} // StockDory

#endif //STOCKDORY_PIECE_H
