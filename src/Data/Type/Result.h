//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_RESULT_H
#define STOCKDORY_RESULT_H
#include <cstdint>

namespace StockDory
{

    enum Result : uint8_t
    {

        Win,
        Loss,
        Draw,
        NAR

    };

} // StockDory

#endif //STOCKDORY_RESULT_H
