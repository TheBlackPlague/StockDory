/*
 * Copyright (c) 2023-2026 Shaheryar Sohail
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef STOCKDORY_PINBITBOARD_H
#define STOCKDORY_PINBITBOARD_H

#include "BitBoard.h"

struct PinBitBoard
{

    BitBoard Straight = BBDefault;
    BitBoard Diagonal = BBDefault;

};

#endif //STOCKDORY_PINBITBOARD_H
