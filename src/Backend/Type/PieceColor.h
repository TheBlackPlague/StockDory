//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PIECECOLOR_H
#define STOCKDORY_PIECECOLOR_H

#include <cstdint>

#include "Piece.h"
#include "Color.h"

struct PieceColor
{

    private:
    // [    COLOR    ] [    PIECE   ]
    // [   4 BITS    ] [   4 BITS   ]
    uint8_t Internal;

    constexpr static uint8_t PieceColorMask = 0x0F;

    public:
    constexpr PieceColor()
    {
        Internal = 0;
    }

    constexpr explicit PieceColor(const Piece piece, const Color color)
    {
        Internal = piece | color << 4;
    }

    [[nodiscard]]
    constexpr inline Piece Piece() const
    {
        return static_cast<enum Piece>(Internal & PieceColorMask);
    }

    [[nodiscard]]
    constexpr inline Color Color() const
    {
        return static_cast<enum Color>(Internal >> 4);
    }

    [[nodiscard]]
    inline std::string ToString() const
    {
        const enum Piece p = Piece();
        const enum Color c = Color();

        return ::ToString(c) + std::string(" ") + ::ToString(p);
    }

};

#endif //STOCKDORY_PIECECOLOR_H
