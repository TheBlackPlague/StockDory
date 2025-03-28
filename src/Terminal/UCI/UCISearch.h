//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCISEARCH_H
#define STOCKDORY_UCISEARCH_H

#include "../../Backend/ThreadPool.h"

#include "../../Engine/Search.h"

namespace StockDory
{

    class UCISearchLogger
    {

        public:
        static void LogDepthIteration(const uint8_t  depth, const uint8_t      selectiveDepth, const int32_t evaluation,
                                      const uint64_t nodes, const uint64_t     ttNodes,
                                      const MS       time,  const std::string& pv)
        {
            std::stringstream output;

            int64_t displayedTime = time.count();
            displayedTime         = std::max(displayedTime, static_cast<int64_t>(1));

            const auto nps = static_cast<uint64_t>(static_cast<double>(nodes) /
             (static_cast<double>(displayedTime) / static_cast<double>(1000)));

            output << "info ";
            output <<    "depth " << static_cast<uint16_t>(         depth) << " ";
            output << "seldepth " << static_cast<uint16_t>(selectiveDepth) << " ";
            output << "score ";

            if (abs(evaluation) > Infinity - MaxDepth)
                output << "mate " << (evaluation > 0 ? Infinity - evaluation : -Infinity - evaluation) / 2 << " ";
            else
                output << "cp " << evaluation << " ";

            output <<   "nodes " <<   nodes << " ";
            output << "ttnodes " << ttNodes << " ";

            output << "time " << displayedTime << " ";
            output << "nps " << nps << " ";
            output << "pv " << pv;

            std::cout << output.str() << std::endl;
        }

        static void LogBestMove(const Move& move)
        {
            std::stringstream output;
            output << "bestmove " << move.ToString();

            std::cout << output.str() << std::endl;
        }

    };

    class UCISearch
    {

        using Search = Search<UCISearchLogger>;

        Search EngineSearch;

        bool Running = false;

        public:
        UCISearch() = default;

        UCISearch(const Board&             board,             const TimeControl& timeControl,
                  const RepetitionHistory& repetitionHistory, const uint8_t      halfMoveCounter)
            : EngineSearch(Search(board, timeControl, repetitionHistory, halfMoveCounter)) {}

        void Start(const Limit limit)
        {
            drjit::do_async([this, limit] -> void
            {
                Running = true ;
                EngineSearch.IterativeDeepening(limit);
                Running = false;
            }, {}, ThreadPool);
        }

        void Stop()
        {
            EngineSearch.ForceStop();
        }

        [[nodiscard]]
        bool IsRunning() const
        {
            return Running;
        }

    };

} // StockDory

#endif //STOCKDORY_UCISEARCH_H
