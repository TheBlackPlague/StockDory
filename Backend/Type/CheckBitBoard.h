//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_CHECKBITBOARD_H
#define STOCKDORY_CHECKBITBOARD_H

#include "BitBoard.h"

struct CheckBitBoard
{

    public:
        BitBoard Check       = BBDefault;
        bool     DoubleCheck = false    ;

};

#endif //STOCKDORY_CHECKBITBOARD_H
