//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_UCISEARCHEVENTHANDLER_H
#define STOCKDORY_UCISEARCHEVENTHANDLER_H

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>

#include "Engine/Engine.h"

#include "Terminal/Output.h"

namespace StockDory
{

    struct UCISearchEventHandler
    {

        bool ShowWDL  = false;
        bool ChessFRC = false;

        void HandleIterationCompletion(const IterationCompletionEvent& event) const
        {
            const int64_t time = std::max<int64_t>(event.Time.count(), 1);

            const auto nps = static_cast<uint64_t>(static_cast<double>(event.Nodes) /
                                                  (static_cast<double>(time       ) / 1000.0));

            std::string line = std::format("info depth {} seldepth {} ", event.Depth, event.SelectiveDepth);

            if (IsMate(event.Evaluation)) {
                const Score ply = PlyToMate(event.Evaluation);

                line += std::format("score mate {} ", ply > 0 ? (ply + 1) / 2 : (ply - 1) / 2);
            } else {
                line += std::format("score cp {} ", event.Evaluation);
            }

            if (ShowWDL) line += std::format("wdl {} {} {} ", event.WDL.W, event.WDL.D, event.WDL.L);

            line += std::format("nodes {} nps {} hashfull {} time {} ", event.Nodes, nps, Engine::HashFull(), time);

            line += "pv ";

            for (uint8_t i = 0; i < event.PVEntry.Ply; i++) {
                if (i != 0) line += " ";

                line += event.Board.ToString(event.PVEntry.PV[i], ChessFRC);
            }

            Print(line);
        }

        void HandleSearchCompletion(const SearchCompletionEvent& event) const
        {
            Print(std::format("bestmove {}", event.Board.ToString(event.Move, ChessFRC)));
        }

    };

} // StockDory

#endif //STOCKDORY_UCISEARCHEVENTHANDLER_H
