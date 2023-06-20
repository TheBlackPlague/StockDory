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

                uint16_t MovesToGo;

            };

        private:
            std::chrono::time_point<std::chrono::high_resolution_clock> Beginning;

            Milliseconds Time = ZeroTime;

            constexpr static uint8_t TPartition = 20;
            constexpr static uint8_t Overhead   = 10;

            constexpr static uint8_t IPartitionN = 3;
            constexpr static uint8_t IPartitionD = 4;
            constexpr static uint8_t TDeltaN     = 3;
            constexpr static uint8_t TDeltaD     = 4;

        public:
            TimeControl() = default;

            explicit TimeControl(const uint64_t time)
            {
                Time = Milliseconds (time);
            }

            explicit TimeControl(const Board& board, const TimeData& timeData)
            {
                const Color color = board.ColorToMove();

                const Milliseconds oT = color == White ? timeData.WhiteTime : timeData.BlackTime;
                const Milliseconds tT = color == White ? timeData.BlackTime : timeData.WhiteTime;

                const Milliseconds oI = color == White ? timeData.WhiteIncrement : timeData.BlackIncrement;

                Time += oT / TPartition;

                if (timeData.MovesToGo > 0)
                    Time = std::max(Time, oT / timeData.MovesToGo);

                Time += oI * IPartitionN / IPartitionD;

                if (oT - Milliseconds(1000) > tT) Time += (oT - tT) * TDeltaN / TDeltaD;

                Time -= Milliseconds(Overhead);
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
