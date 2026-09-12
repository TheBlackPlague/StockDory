//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_COLOR_H
#define STOCKDORY_COLOR_H

#include <cstdint>
#include <string_view>

#include "../Misc.h"

enum Color : uint8_t
{

    White,
    Black,
    NAC

};

constexpr Color Next(const Color c)
{
    return static_cast<Color>(static_cast<uint8_t>(c) +   1);
}

constexpr Color Opposite(const Color c)
{
    return static_cast<Color>(static_cast<uint8_t>(c) ^ 0x1);
}

constexpr Array<std::string_view, 3> C_STRING {
    "White",
    "Black",
    "NAC"
};

inline std::string ToString(const Color c)
{
    return std::string(C_STRING[c]);
}

#endif //STOCKDORY_COLOR_H
