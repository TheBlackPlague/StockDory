//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_UCITIME_H
#define STOCKDORY_UCITIME_H

#include "../../Engine/Search.h"

namespace StockDory
{

    template<bool Fixed>
    struct UCITime {};

    template<>
    struct UCITime<true>
    {

        uint64_t Time = 0;

        void AsLimit(Limit& limit) const
        {
            limit.Timed = true;
            limit.Fixed = true;

            limit. ActualTime = MS(Time);
            limit.   BaseTime = MS(Time);
            limit.OptimalTime = MS(Time);
        }

    };

    template<>
    struct UCITime<false>
    {

        uint64_t WhiteTime = 0;
        uint64_t BlackTime = 0;
        uint64_t WhiteInc  = 0;
        uint64_t BlackInc  = 0;
        uint64_t MovesToGo = 0;

        Color ColorToMove = NAC;

        void AsLimit(Limit& limit) const
        {
            limit.Timed = true ;
            limit.Fixed = false;

            const uint64_t time = ColorToMove == White ? WhiteTime : BlackTime;
            const uint64_t inc  = ColorToMove == White ? WhiteInc  : BlackInc ;

            const uint64_t usableTime = time > TimeProcessingOverhead ? time - TimeProcessingOverhead : 0;

            const uint64_t movesToGo = MovesToGo > 0 ? MovesToGo : TimeDefaultMovesToGo;

            const uint64_t timePerMove = usableTime / movesToGo + (usableTime % movesToGo != 0);

            const uint64_t incrementTime = inc / TimeIncrementPartitionDenominator * TimeIncrementPartitionNumerator +
                                           inc % TimeIncrementPartitionDenominator * TimeIncrementPartitionNumerator /
                                                 TimeIncrementPartitionDenominator;

            const uint64_t optimalTime = timePerMove + std::min<uint64_t>(incrementTime, usableTime - timePerMove);
            const uint64_t  actualTime = optimalTime > usableTime / TimeHardLimitMultiplier ?
                                                       usableTime : optimalTime * TimeHardLimitMultiplier;

            limit. ActualTime = MS( actualTime);
            limit.   BaseTime = MS(optimalTime);
            limit.OptimalTime = MS(optimalTime);
        }

    };

} // StockDory

#endif //STOCKDORY_UCITIME_H
