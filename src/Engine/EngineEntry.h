//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_ENGINEENTRY_H
#define STOCKDORY_ENGINEENTRY_H

#include "../Backend/Type/Zobrist.h"
#include "../Backend/Type/Move.h"

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

        using Generation = uint16_t;

        ZobristHash     Hash       = 0;
        int16_t         Evaluation = 0;
        Move            Move       = ::Move();
        Generation      Gen        = 0;
        uint8_t         Depth      = 0;
        EngineEntryType Type       = Invalid;

        [[nodiscard]]
        constexpr uint16_t Quality() const
        {
            return Gen + Depth / 5;
        }

    };

} // StockDory

#endif //STOCKDORY_ENGINEENTRY_H
