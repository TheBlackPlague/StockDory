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

        ZobristHash     Hash       = 0;
        int32_t         Evaluation = 0;
        Move            Move       = ::Move();
        uint8_t         Depth      = 0;
        EngineEntryType Type       = Invalid;

        constexpr inline bool operator>(const EngineEntry& old) const
        {
            constexpr uint8_t ReplacementThreshold = 3;

            if (Type == Exact) return true;

            if (Hash != old.Hash) return true;

            if (Type == BetaCutoff && old.Type == AlphaUnchanged) return true;

            if (Depth > old.Depth - ReplacementThreshold) return true;

            return false;
        }

    };

} // StockDory

#endif //STOCKDORY_ENGINEENTRY_H
