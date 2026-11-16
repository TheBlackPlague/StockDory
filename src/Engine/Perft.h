//
// Copyright (c) 2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_PERFT_H
#define STOCKDORY_PERFT_H

#include <chrono>
#include <cstdint>
#include <numeric>

#include "MoveList.h"
#include "ThreadPool.h"

namespace StockDory
{

    struct PerftMode
    {

        bool Bulk     =  true;
        bool Parallel = false;

    };

    constexpr uint8_t PerftSplitDepth = 5;

    template<Side Us, PerftMode Mode>
    uint64_t Perft(Board& board, const uint8_t depth)
    {
        constexpr Side Them = Opposite(Us);

        constexpr PerftMode Serial = { .Bulk = Mode.Bulk, .Parallel = false };

        if (depth == 1 && Mode.Bulk) return MoveCount<Us>(board);

        const MoveList<Us> moves (board);

        if (Mode.Parallel && depth >= PerftSplitDepth) {
            Array<uint64_t, MaxMove> nodes;

            ThreadPool.For(
                moves.Count(),
                [&](const size_t i) -> void
                {
                    Board copy = board;

                    copy.MakeMove<Plain>(moves[i]);

                    nodes[i] = depth - 1 >= PerftSplitDepth ? Perft<Them, Mode  >(copy, depth - 1)
                                                            : Perft<Them, Serial>(copy, depth - 1);
                }
            );

            return std::accumulate(nodes.begin(), nodes.begin() + moves.Count(), 0ULL);
        }

        uint64_t nodes = 0;

        for (const Move move : moves) {
            const auto state = board.MakeMove<Plain>(move);

            nodes += depth == 1 ? 1 : Perft<Them, Serial>(board, depth - 1);

            board.UndoMove<Plain>(state, move);
        }

        return nodes;
    }

    template<Side Us, PerftMode Mode, typename OnMove>
    uint64_t PerftRoot(const Board& board, const uint8_t depth, const OnMove& onMove)
    {
        constexpr Side Them = Opposite(Us);

        constexpr PerftMode Serial = { .Bulk = Mode.Bulk, .Parallel = false };

        const MoveList<Us> moves (board);

        Array<uint64_t, MaxMove> nodes;

        const auto Item = [&](const size_t i) -> void
        {
            if (depth == 1) nodes[i] = 1;
            else {
                Board copy = board;

                copy.MakeMove<Plain>(moves[i]);

                nodes[i] = depth - 1 >= PerftSplitDepth ? Perft<Them, Mode  >(copy, depth - 1)
                                                        : Perft<Them, Serial>(copy, depth - 1);
            }

            onMove(moves[i], nodes[i]);
        };

        if (Mode.Parallel) ThreadPool.For(moves.Count(), Item);
        else               for (size_t i = 0; i < moves.Count(); i++) Item(i);

        return std::accumulate(nodes.begin(), nodes.begin() + moves.Count(), 0ULL);
    }

    template<Side Us, typename OnMove>
    uint64_t PerftRoot(const Board& board, const uint8_t depth, const PerftMode mode, const OnMove& onMove)
    {
        // ReSharper disable CppUseDesignatedInitializers
        if ( mode.Bulk && !mode.Parallel) return PerftRoot<Us, PerftMode { true , false }>(board, depth, onMove);
        if ( mode.Bulk &&  mode.Parallel) return PerftRoot<Us, PerftMode { true , true  }>(board, depth, onMove);
        if (!mode.Bulk && !mode.Parallel) return PerftRoot<Us, PerftMode { false, false }>(board, depth, onMove);

        return PerftRoot<Us, PerftMode { false, true }>(board, depth, onMove);
        // ReSharper restore CppUseDesignatedInitializers
    }

    struct PerftResult
    {

        uint64_t Nodes = 0;

        std::chrono::nanoseconds Time {};

    };

} // StockDory

#endif //STOCKDORY_PERFT_H
