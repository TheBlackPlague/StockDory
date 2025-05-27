//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCITIME_H
#define STOCKDORY_UCITIME_H

#include "../../Engine/Search.h"

namespace StockDory
{

    template<typename Derived>
    struct UCITimeBase
    {

        Limit AsLimit() const { Limit limit; return static_cast<const Derived*>(this)->AsLimit(limit); return limit; }

    };

    template<bool Fixed>
    struct UCITime : UCITimeBase<UCITime<Fixed>> {};

    template<>
    struct UCITime<true> : UCITimeBase<UCITime<true>>
    {

        uint64_t Time = 0;

        void AsLimit(Limit& limit) const
        {
            limit.Timed = true;
            limit.Fixed = true;

            limit.ActualTime  = MS(Time);
            limit.OptimalTime = limit.ActualTime;
        }

    };

    template<>
    struct UCITime<false> : UCITimeBase<UCITime<false>>
    {

        uint64_t WhiteTime  = 0;
        uint64_t BlackTime  = 0;
        uint64_t WhiteInc   = 0;
        uint64_t BlackInc   = 0;

        uint16_t MovesToGo  = 0;

        Color ColorToMove = NAC;

        void AsLimit(Limit& limit) const
        {
            limit.Timed = true ;
            limit.Fixed = false;

            const uint64_t time = ColorToMove == White ? WhiteTime : BlackTime;
            const uint64_t inc  = ColorToMove == White ? WhiteInc  : BlackInc ;

            uint64_t actualTime = time * TimeBasePartitionNumerator / TimeBasePartitionDenominator;

            actualTime = MovesToGo > 0 ? std::max(actualTime, time / MovesToGo) : actualTime;

            actualTime += inc * TimeIncrementPartitionNumerator / TimeIncrementPartitionDenominator;

            actualTime -= TimeProcessingOverhead;

            limit.ActualTime  =    MS(actualTime);
            limit.OptimalTime = limit.ActualTime ;
        }

    };

} // StockDory

#endif //STOCKDORY_UCITIME_H
