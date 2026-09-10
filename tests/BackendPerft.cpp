//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <utility>

#include "Backend/Misc.h"
#include "Terminal/Perft/PerftRunner.h"

#include "PerftPositions.h"

#ifndef STOCKDORY_TEST_BOARD_TYPE
#define STOCKDORY_TEST_BOARD_TYPE StockDory::Board
#endif

using TestBoard = STOCKDORY_TEST_BOARD_TYPE;

uint64_t Count(TestBoard& board, const uint8_t depth, const bool sync)
{
    // The frozen baseline predates the depth-zero entry-point fix.
#ifdef STOCKDORY_BASELINE
    if (depth == 0) return 1;
#endif

    if (sync)
        return board.ColorToMove() == White
            ? StockDory::PerftRunner::Perft<White, false, true>(board, depth)
            : StockDory::PerftRunner::Perft<Black, false, true>(board, depth);

    return board.ColorToMove() == White
        ? StockDory::PerftRunner::Perft<White, false, false>(board, depth)
        : StockDory::PerftRunner::Perft<Black, false, false>(board, depth);
}

int main(const int argc, const char* argv[])
{
    uint8_t maximumDepth = 7;
    size_t threads = 1;
    std::string filter;

    for (int i = 1; i < argc; i++) {
        const std::string option = argv[i];

        if (option == "--max-depth" && i + 1 < argc) {
            const int depth = std::stoi(argv[++i]);
            if (depth < 0 || depth > 7) return 2;
            maximumDepth = static_cast<uint8_t>(depth);
        } else if (option == "--position" && i + 1 < argc) {
            filter = argv[++i];
        } else if (option == "--threads" && i + 1 < argc) {
            threads = std::stoul(argv[++i]);
            if (threads == 0 || threads > 256) return 2;
        } else {
            std::cerr << "Usage: BackendPerft [--max-depth 0..7] [--position name] [--threads count]\n";
            return 2;
        }
    }

    size_t tested = 0;
    size_t failed = 0;

    StockDory::ThreadPool.Resize(threads);

    std::cout << std::setprecision(9);

    for (const auto& position : StockDory::Testing::PerftPositions) {
        if (!filter.empty() && filter != position.Name) continue;

        for (uint8_t depth = 0; depth <= std::min(maximumDepth, position.Depth); depth++) {
            TestBoard board (std::string(position.Fen));
            const auto fen  = board.Fen();
            const auto hash = board.Zobrist();

            const auto start = std::chrono::steady_clock::now();
            const uint64_t nodes = Count(board, depth, threads == 1);
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

            const bool restored = board.Fen() == fen && board.Zobrist() == hash;
            const bool passed = nodes == position.Nodes[depth] && restored;
            tested++;
            failed += !passed;

            std::cout << "{\"position\":\"" << position.Name << "\",\"depth\":" << static_cast<int>(depth)
                      << ",\"threads\":" << threads
                      << ",\"nodes\":" << nodes << ",\"expected\":" << position.Nodes[depth]
                      << ",\"seconds\":" << seconds << ",\"restored\":" << (restored ? "true" : "false")
                      << ",\"passed\":" << (passed ? "true" : "false") << "}" << std::endl;
        }
    }

    std::cerr << tested << " checks, " << failed << " failures\n";
    return tested > 0 && failed == 0 ? 0 : 1;
}
