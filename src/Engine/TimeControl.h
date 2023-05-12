//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TIMECONTROL_H
#define STOCKDORY_TIMECONTROL_H

#include <chrono>

#include "../Backend/Board.h"

namespace StockDory
{

    class TimeControl
    {

        public:
            using Milliseconds = std::chrono::milliseconds;

            constexpr static Milliseconds ZeroTime = Milliseconds(0);

            struct TimeData
            {

                Milliseconds WhiteTime;
                Milliseconds BlackTime;
                Milliseconds WhiteIncrement;
                Milliseconds BlackIncrement;

            };

        private:
            std::chrono::time_point<std::chrono::high_resolution_clock> Beginning;

            Milliseconds Time = ZeroTime;

            constexpr static uint8_t Partition = 20;

        public:
            TimeControl() = default;

            explicit TimeControl(const uint64_t time)
            {
                Time = Milliseconds (time);
            }

            explicit TimeControl(const Board& board, const TimeData& timeData)
            {
                Time =  board.ColorToMove() == White ? timeData.WhiteTime      : timeData.BlackTime     ;

                Time /= Partition;

                Time += board.ColorToMove() == White ? timeData.WhiteIncrement : timeData.BlackIncrement;
            }

            inline void Start()
            {
                Beginning = std::chrono::high_resolution_clock::now();
            }

            [[nodiscard]]
            inline Milliseconds SinceBeginning() const
            {
                std::chrono::duration duration = std::chrono::high_resolution_clock::now() - Beginning;
                return std::chrono::duration_cast<Milliseconds>(duration);
            }

            [[nodiscard]]
            inline bool Finished() const
            {
                if (Time == ZeroTime) return false;

                return SinceBeginning() > Time;
            }

    };

} // StockDory

#endif //STOCKDORY_TIMECONTROL_H
