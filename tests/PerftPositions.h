//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_PERFTPOSITIONS_H
#define STOCKDORY_PERFTPOSITIONS_H

#include <array>
#include <cstdint>
#include <string_view>

namespace StockDory::Testing
{

    struct PerftPosition
    {

        std::string_view Name;
        std::string_view Fen;
        std::array<uint64_t, 8> Nodes;
        uint8_t Depth;
        uint8_t BenchmarkDepth;

    };

    // Published counts: https://www.chessprogramming.org/Perft_Results
    constexpr std::array<PerftPosition, 7> PerftPositions {{
        {
            "initial", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            {1, 20, 400, 8902, 197281, 4865609, 119060324, 3195901860}, 7, 5
        },
        {
            "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            {1, 48, 2039, 97862, 4085603, 193690690, 8031647685}, 6, 4
        },
        {
            "position-3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            {1, 14, 191, 2812, 43238, 674624, 11030083, 178633661}, 7, 6
        },
        {
            "position-4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            {1, 6, 264, 9467, 422333, 15833292, 706045033}, 6, 5
        },
        {
            "position-4-mirrored", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
            {1, 6, 264, 9467, 422333, 15833292, 706045033}, 6, 5
        },
        {
            "position-5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            {1, 44, 1486, 62379, 2103487, 89941194}, 5, 4
        },
        {
            "position-6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
            {1, 46, 2079, 89890, 3894594, 164075551, 6923051137, 287188994746}, 7, 4
        }
    }};

} // StockDory::Testing

#endif // STOCKDORY_PERFTPOSITIONS_H
