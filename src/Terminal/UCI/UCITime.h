//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_UCITIME_H
#define STOCKDORY_UCITIME_H

#include <cmath>

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

        uint16_t FullMoveNumber = 1;

        Color ColorToMove = NAC;

        void AsLimit(Limit& limit) const
        {
            limit.Timed = true ;
            limit.Fixed = false;

            const uint64_t time = ColorToMove == White ? WhiteTime : BlackTime;
            const uint64_t inc  = ColorToMove == White ? WhiteInc  : BlackInc ;

            const uint64_t usableTime = time > TimeProcessingOverhead ? time - TimeProcessingOverhead : 0;

            const uint64_t incTime = inc / TimeIncrementPartitionDenominator * TimeIncrementPartitionNumerator +
                                     inc % TimeIncrementPartitionDenominator * TimeIncrementPartitionNumerator /
                                           TimeIncrementPartitionDenominator                                   ;

            const uint64_t clkTime = usableTime / TimeHardLimitClockDenominator * TimeHardLimitClockNumerator +
                                     usableTime % TimeHardLimitClockDenominator * TimeHardLimitClockNumerator /
                                                  TimeHardLimitClockDenominator                               ;

            uint64_t baseTimeForMove;

            if (MovesToGo > 0) {
                baseTimeForMove = usableTime / MovesToGo + (usableTime % MovesToGo != 0);
            } else {
                const double exp = std::exp(-TimeSoftFractionRate / 1000.0 * FullMoveNumber);

                const double fraction = (TimeSoftFractionMax - TimeSoftFractionDecay * exp) / 1000.0;

                baseTimeForMove = static_cast<uint64_t>(static_cast<double>(usableTime) * std::max(fraction, 0.0));
            }

            const uint64_t allocatedTime = baseTimeForMove + std::min<uint64_t>(incTime, usableTime - baseTimeForMove);

            const uint64_t  actualTime = std::min<uint64_t>(allocatedTime * TimeHardLimitMultiplier,    clkTime);
            const uint64_t optimalTime = std::min<uint64_t>(allocatedTime                          , actualTime);

            limit. ActualTime = MS( actualTime);
            limit.   BaseTime = MS(optimalTime);
            limit.OptimalTime = MS(optimalTime);
        }

    };

} // StockDory

#endif //STOCKDORY_UCITIME_H
