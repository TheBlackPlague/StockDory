//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PIECECOLOR_H
#define STOCKDORY_PIECECOLOR_H

#include <cstdint>

#include "Color.h"
#include "Piece.h"

struct PieceColor
{

    private:
    constexpr static uint8_t PieceColorMask = 0xF;
    constexpr static uint8_t      ColorPos  =   4;

    // [    COLOR    ] [    PIECE   ]
    // [   4 BITS    ] [   4 BITS   ]
    uint8_t Internal;

    public:
    constexpr PieceColor()
    {
        Internal = 0;
    }

    constexpr PieceColor(const Piece piece, const Color color)
    {
        Internal = piece | color << 4;
    }

    [[nodiscard]]
    constexpr Piece Piece() const
    {
        return static_cast<enum Piece>(Internal & PieceColorMask);
    }

    [[nodiscard]]
    constexpr Color Color() const
    {
        return static_cast<enum Color>(Internal >> ColorPos);
    }

    [[nodiscard]]
    std::string ToString() const
    {
        const enum Piece p = Piece();
        const enum Color c = Color();

        return ::ToString(c) + std::string(" ") + ::ToString(p);
    }

};

#endif //STOCKDORY_PIECECOLOR_H
