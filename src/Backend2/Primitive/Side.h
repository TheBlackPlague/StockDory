//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SIDE_H
#define STOCKDORY_SIDE_H

#include "Base.h"

namespace StockDory
{

    enum Side : u08 { White, Black, InvalidSide };

    constexpr void operator ++(Side& side) { side = static_cast<Side>(side + 1); }

    constexpr inline Array<String, 3> SideToString {
        "White",
        "Black",
        "Invalid"
    };

    OutputStream& operator <<(OutputStream& os, const Side side) { return os << SideToString[side]; }

} // StockDory

#endif //STOCKDORY_SIDE_H
