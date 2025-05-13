//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PINBITBOARD_H
#define STOCKDORY_PINBITBOARD_H

#include "BitBoard.h"

struct PinBitBoard
{

    BitBoard Straight = BBDefault;
    BitBoard Diagonal = BBDefault;

};

#endif //STOCKDORY_PINBITBOARD_H
