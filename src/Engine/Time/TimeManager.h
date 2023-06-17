//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TIMEMANAGER_H
#define STOCKDORY_TIMEMANAGER_H

#include "TimeControl.h"

#include "../../Backend/Board.h"

namespace StockDory
{

    struct TimeData
    {

        uint64_t WhiteTime     ;
        uint64_t BlackTime     ;
        uint64_t WhiteIncrement;
        uint64_t BlackIncrement;
        uint16_t MovesToGo     ;

    };

    class TimeManager
    {

        private:
            constexpr static uint8_t TimePartition = 20;
            constexpr static uint8_t TimeOverhead  = 10;

            constexpr static uint8_t IncrementPartitionNumerator   = 3;
            constexpr static uint8_t IncrementPartitionDenominator = 4;

            constexpr static uint8_t OptimalNumerator   = 5;
            constexpr static uint8_t OptimalDenominator = 5;

        public:
            static TimeControl Default()
            {
                return {};
            }

            static TimeControl Fixed(const uint64_t time)
            {
                return TimeControl(time, time);
            }

            static TimeControl Optimal(const Board& board, const TimeData& data)
            {
                const Color color = board.ColorToMove();

                const uint64_t time = color == White ? data.WhiteTime      : data.BlackTime     ;
                const uint64_t inc  = color == White ? data.WhiteIncrement : data.BlackIncrement;

                uint64_t actual = time / TimePartition;

                actual = data.MovesToGo > 0 ? std::max(actual, time / data.MovesToGo) : actual;

                actual += inc * IncrementPartitionNumerator / IncrementPartitionDenominator;

                actual -= TimeOverhead;

                const uint64_t optimal = actual * OptimalNumerator / OptimalDenominator;

                return TimeControl(optimal, actual);
            }

    };

} // StockDory

#endif //STOCKDORY_TIMEMANAGER_H
