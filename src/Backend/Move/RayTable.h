//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_RAYTABLE_H
#define STOCKDORY_RAYTABLE_H

#include "../Type/BitBoard.h"

namespace StockDory::RayTable
{

    constexpr std::array<BitBoard, 8> Horizontal {
        0x0101010101010101, 0x0202020202020202, 0x0404040404040404, 0x0808080808080808,
        0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080
    };

    constexpr std::array<BitBoard, 8> Vertical {
        0x00000000000000FF, 0x000000000000FF00, 0x0000000000FF0000, 0x00000000FF000000,
        0x000000FF00000000, 0x0000FF0000000000, 0x00FF000000000000, 0xFF00000000000000
    };

    constexpr BitBoard Edged = Horizontal[0] | Horizontal[7] | Vertical[0] | Vertical[7];

    extern std::array<std::array<BitBoard, 64>, 64> Between;

} // StockDory

#endif //STOCKDORY_RAYTABLE_H
