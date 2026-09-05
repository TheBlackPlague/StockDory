/*
 * Copyright (c) 2023-2026 Shaheryar Sohail
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef STOCKDORY_CHECKBITBOARD_H
#define STOCKDORY_CHECKBITBOARD_H

#include "BitBoard.h"

struct CheckBitBoard
{

    BitBoard Check       = BBDefault;
    bool     DoubleCheck = false;

};

#endif //STOCKDORY_CHECKBITBOARD_H
