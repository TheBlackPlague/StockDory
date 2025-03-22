//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_ENGINEENTRY_H
#define STOCKDORY_ENGINEENTRY_H

#include "../Backend/Type/Move.h"
#include "../Backend/Type/Zobrist.h"

namespace StockDory
{

    enum EngineEntryType : uint8_t
    {

        Exact,
        BetaCutoff,
        AlphaUnchanged,
        Invalid

    };

    struct EngineEntry
    {

        ZobristHash     Hash       = 0;
        int32_t         Evaluation = 0;
        Move            Move       = ::Move();
        uint8_t         Depth      = 0;
        EngineEntryType Type       = Invalid;

    };

} // StockDory

#endif //STOCKDORY_ENGINEENTRY_H
