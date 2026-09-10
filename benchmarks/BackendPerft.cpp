//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "Backend/Misc.h"
#include "Terminal/Perft/PerftRunner.h"

#include "../tests/PerftPositions.h"

#ifndef STOCKDORY_TEST_BOARD_TYPE
#define STOCKDORY_TEST_BOARD_TYPE StockDory::Board
#endif

using TestBoard = STOCKDORY_TEST_BOARD_TYPE;

int main(const int argc, const char* argv[])
{
    size_t epochs = 31;
    uint64_t iterations = 16;
    std::string filter;

    for (int i = 1; i < argc; i++) {
        const std::string option = argv[i];

        if (option == "--epochs" && i + 1 < argc) epochs = std::stoul(argv[++i]);
        else if (option == "--iterations" && i + 1 < argc) iterations = std::stoull(argv[++i]);
        else if (option == "--position" && i + 1 < argc) filter = argv[++i];
        else {
            std::cerr << "Usage: BackendPerftBenchmark [--epochs count] [--iterations count] [--position name]\n";
            return 2;
        }
    }

    if (epochs == 0 || iterations == 0) return 2;

    ankerl::nanobench::Bench benchmark;
    benchmark.title("StockDory single-thread PERFT").output(nullptr).epochs(epochs)
             .epochIterations(iterations).warmup(0).performanceCounters(false);

    size_t tested = 0;

    for (const auto& position : StockDory::Testing::PerftPositions) {
        if (!filter.empty() && filter != position.Name) continue;

        TestBoard board (std::string(position.Fen));
        const uint8_t depth = position.BenchmarkDepth;
        const uint64_t expected = position.Nodes[depth];
        const auto fen = board.Fen();
        const auto hash = board.Zobrist();

        const auto traverse = [&] -> void
        {
            const uint64_t nodes = board.ColorToMove() == White
                ? StockDory::PerftRunner::Perft<White, false, true>(board, depth)
                : StockDory::PerftRunner::Perft<Black, false, true>(board, depth);

            ankerl::nanobench::doNotOptimizeAway(nodes);
            if (nodes != expected) throw std::runtime_error("Incorrect PERFT count");
        };

        // nanobench 4.3.11 recalibrates its first epoch when its built-in warmup is enabled.
        traverse();
        traverse();
        benchmark.batch(expected).unit("node").run(std::string(position.Name), traverse);

        if (board.Fen() != fen || board.Zobrist() != hash)
            throw std::runtime_error("PERFT did not restore its board");

        tested++;
    }

    if (tested == 0) return 1;
    ankerl::nanobench::render(ankerl::nanobench::templates::json(), benchmark, std::cout);
}
