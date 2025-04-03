//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEED_H
#define STOCKDORY_SEED_H

#include "PackedPosition.h"
#include "Result.h"

namespace StockDory
{

    struct Seed
    {

        private:
        using PositionScore = int16_t;

        public:
        PackedPosition  Position    = PackedPosition();
        Move           FirstMove    =           Move();
        PositionScore      Score    =                0;
        Result            Result: 2 =              NAR;
        uint8_t       StalkCount: 6 =                0;

        std::array<uint8_t, 3> __PADDING = {};

    };

} // StockDory

#endif //STOCKDORY_SEED_H
