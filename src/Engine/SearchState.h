//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEARCHSTATE_H
#define STOCKDORY_SEARCHSTATE_H

#include "../Backend/Type/Move.h"
#include "../Backend/Type/Zobrist.h"

#include "Common.h"

namespace StockDory
{

    enum SearchStateType : uint8_t
    {

        Exact,
        BetaCutoff,
        AlphaUnchanged,
        Invalid

    };

    using CompressedHash  = uint16_t;
    using CompressedScore =  int16_t;

    constexpr CompressedScore CompressedInfinity = 32000;

    CompressedHash  CompressHash (const ZobristHash hash) { return static_cast<CompressedHash >(hash ); }
    CompressedScore CompressScore(const Score      score) { return static_cast<CompressedScore>(score); }

    struct SearchState
    {

        CompressedHash  Hash       = 0;
        CompressedScore Evaluation = 0;
        Move            Move       = ::Move();
        uint8_t         Depth      = 0;
        SearchStateType Type       = Invalid;

    };

} // StockDory

#endif //STOCKDORY_SEARCHSTATE_H
