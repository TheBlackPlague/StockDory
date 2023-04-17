//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TABLESETUP_H
#define STOCKDORY_TABLESETUP_H

#include "AttackTable.h"
#include "UtilityTable.h"

inline void TableSetup()
{
    // Initialize Sliding:
    StockDory::AttackTable ::InitializeSliding();

    // Initialize Between:
    StockDory::UtilityTable::InitializeBetween();
}

#endif //STOCKDORY_TABLESETUP_H
