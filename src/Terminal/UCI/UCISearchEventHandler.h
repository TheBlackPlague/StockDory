//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCISEARCHEVENTHANDLER_H
#define STOCKDORY_UCISEARCHEVENTHANDLER_H

#include "../../Engine/Search.h"

namespace StockDory
{

    class UCISearchEventHandler : DefaultSearchEventHandler
    {

        static inline bool OutputWDL = false;

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
        static void HandleIterativeDeepeningIterationCompletion(const IterativeDeepeningIterationCompletionEvent& event)
        {
            std::stringstream output;

            int64_t displayedTime = event.Time.count();
            displayedTime         = std::max(displayedTime, static_cast<int64_t>(1));

            const auto nps = static_cast<uint64_t>(static_cast<double>(event.Nodes  ) /
                                                  (static_cast<double>(displayedTime) / 1000.0));

            output << "info depth " << static_cast<uint16_t>(event.         Depth) << " ";
            output <<   "seldepth " << static_cast<uint16_t>(event.SelectiveDepth) << " ";

            output << "score ";

            if (IsMate(event.Evaluation)) {
                output << "mate " << (PlyToMate(event.Evaluation) + 1) / 2 << " ";
            } else {
                output << "cp " << event.Evaluation << " ";
            }

            if (OutputWDL) output << "wdl " << event.WDL.W << " " << event.WDL.D << " " << event.WDL.L << " ";

            output << "nodes " << event.Nodes << " ";
            output << "nps " << nps << " ";
            output << "time " << displayedTime << " ";
            output << "pv " << PVLine(event.PVEntry);

            std::cout << output.str() << std::endl;
        }

        static void HandleIterativeDeepeningCompletion(const IterativeDeepeningCompletionEvent& event)
        {
            std::cout << "bestmove " << event.Move.ToString() << std::endl;
        }

        static void SetOutputWDL(const bool value) { OutputWDL = value; }

    };

} // StockDory

#endif //STOCKDORY_UCISEARCHEVENTHANDLER_H
