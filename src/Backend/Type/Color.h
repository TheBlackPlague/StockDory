//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_COLOR_H
#define STOCKDORY_COLOR_H

#include <cstdint>

enum Color : uint8_t
{

    White,
    Black,
    NAC

};

inline constexpr Color Next(const Color c)
{
    return static_cast<Color>(static_cast<uint8_t>(c) + 1);
}

inline constexpr Color Opposite(const Color c)
{
    return static_cast<Color>(static_cast<uint8_t>(c) ^ 0x1);
}

#endif //STOCKDORY_COLOR_H
