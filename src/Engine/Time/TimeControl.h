//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TIMECONTROL_H
#define STOCKDORY_TIMECONTROL_H

#include <chrono>

namespace StockDory
{

    using MS = std::chrono::milliseconds;
    using TP = std::chrono::time_point<std::chrono::high_resolution_clock>;

    class TimeControl
    {

        public:
            constexpr static MS Zero = MS(0);

        private:
            TP Origin;

            MS OptimalTime = Zero;
            MS  ActualTime = Zero;

        public:
            TimeControl() = default;

            explicit TimeControl(const uint64_t Optimal, const uint64_t Actual)
            : Origin(std::chrono::high_resolution_clock::now()),
              OptimalTime(MS(Optimal)),
               ActualTime(MS(Actual )) {}

            void Start()
            {
                Origin = std::chrono::high_resolution_clock::now();
            }

            template<bool Hard>
            [[nodiscard]]
            inline bool Finished() const
            {
                if (ActualTime == Zero) return false;

                return Elapsed() > (Hard ? ActualTime : OptimalTime);
            }

            [[nodiscard]]
            inline MS Elapsed() const
            {
                return std::chrono::duration_cast<MS>(std::chrono::high_resolution_clock::now() - Origin);
            }

    };

} // StockDory

#endif //STOCKDORY_TIMECONTROL_H