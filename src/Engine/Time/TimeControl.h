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
    using TP = std::chrono::time_point<std::chrono::steady_clock>;

    class TimeControl
    {

        public:
        constexpr static auto Zero = MS(0);

        private:
        TP Origin;

        MS OptimalTime = Zero;
        MS ActualTime  = Zero;

        bool Optimizable = false;

        public:
        TimeControl() = default;

        TimeControl(const uint64_t optimal, const uint64_t actual, const bool optimizable = false)
        : Origin(std::chrono::steady_clock::now()),
          OptimalTime(MS(optimal)),
           ActualTime(MS( actual)),
          Optimizable(optimizable) {}

        void Start()
        {
            Origin = std::chrono::steady_clock::now();
        }

        [[nodiscard]]
        bool CanBeOptimised() const
        {
            return Optimizable;
        }

        [[nodiscard]]
        uint64_t GetOptimal() const
        {
            return OptimalTime.count();
        }

        void SetOptimal(const uint64_t time)
        {
            const uint64_t adjustedTime = std::min<uint64_t>(time, ActualTime.count());
            OptimalTime                 = MS(adjustedTime);
        }

        template<bool Hard>
        [[nodiscard]]
        bool Finished() const
        {
            if (ActualTime == Zero) return false;

            return Elapsed() > (Hard ? ActualTime : OptimalTime);
        }

        [[nodiscard]]
        MS Elapsed() const
        {
            return std::chrono::duration_cast<MS>(std::chrono::steady_clock::now() - Origin);
        }

    };

} // StockDory

#endif //STOCKDORY_TIMECONTROL_H
