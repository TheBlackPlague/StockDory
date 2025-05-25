//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCISEARCHEVENTHANDLER_H
#define STOCKDORY_UCISEARCHEVENTHANDLER_H

#include "../../Engine/Search2.h"

namespace StockDory
{

    class UCISearchEventHandler : DefaultSearchEventHandler
    {

        static std::string PVLine(const PVEntry& pv)
        {
            std::stringstream line;

            for (uint8_t i = 0; i < pv.Ply; i++) {
                line << pv.PV[i].ToString();

                if (i != pv.Ply - 1) line << " ";
            }

            return line.str();
        }

        public:
        // ReSharper disable once CppMemberFunctionMayBeStatic
        void HandleIterativeDeepeningIterationCompletion(
            [[maybe_unused]] const IterativeDeepeningIterationCompletionEvent& event
        )
        {
            std::stringstream output;

            int64_t displayedTime = event.Time.count();
            displayedTime         = std::max(displayedTime, static_cast<int64_t>(1));

            const auto nps = static_cast<uint64_t>(static_cast<double>(event.Nodes  ) /
                                                  (static_cast<double>(displayedTime) / 1000.0));

            output << "info depth " << static_cast<uint16_t>(event.         Depth) << " ";
            output <<   "seldepth " << static_cast<uint16_t>(event.SelectiveDepth) << " ";

            output << "score ";


            if (abs(event.Evaluation) > Infinity - MaxDepth) {
                const auto mateScore = ((event.Evaluation > 0 ? Infinity : -Infinity) - event.Evaluation) / 2;

                output << "mate " << mateScore << " ";
            } else {
                output << "cp " << event.Evaluation << " ";
            }

            output << "nodes " << event.Nodes << " ";
            output << "nps " << nps << " ";
            output << "time " << displayedTime << " ";
            output << "pv " << PVLine(event.PVEntry);

            std::cout << output.str() << std::endl;
        }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        void HandleIterativeDeepeningCompletion(
            [[maybe_unused]] const IterativeDeepeningCompletionEvent& event
        )
        {
            std::cout << "bestmove " << event.Move.ToString() << std::endl;
        }

    };

} // StockDory

#endif //STOCKDORY_UCISEARCHEVENTHANDLER_H
