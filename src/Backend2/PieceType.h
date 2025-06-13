//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PIECETYPE_H
#define STOCKDORY_PIECETYPE_H

#include "Base.h"

namespace StockDory
{

    enum PieceType : u08 { Pawn, Knight, Bishop, Rook, Queen, King, InvalidPieceType };

    constexpr PieceType operator ++(PieceType& type) { type = static_cast<PieceType>(type + 1); return type; }

    constexpr PieceType operator ++(PieceType& type, int)
    {
        const PieceType temp = type;

        type = static_cast<PieceType>(type + 1);

        return temp;
    }

    OutputStream& operator <<(OutputStream& os, const PieceType type)
    {
        constexpr static Array<String, 7> PieceTypeToString {
            "Pawn"  ,
            "Knight",
            "Bishop",
            "Rook"  ,
            "Queen" ,
            "King"  ,
            "Invalid"
        };

        return os << PieceTypeToString[type];
    }

} // StockDory

#endif //STOCKDORY_PIECETYPE_H
