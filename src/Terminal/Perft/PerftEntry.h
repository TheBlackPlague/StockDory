//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PERFTENTRY_H
#define STOCKDORY_PERFTENTRY_H

#include <bit>
#include <mutex>

#include "../../Backend/TranspositionTable.h"
#include "../../Backend/Type/Zobrist.h"

namespace StockDory::Perft
{

    template<uint8_t Depth>
    struct PerftEntry
    {

        static_assert(Depth <= 64, "Depth must be less than or equal to 64.");

        ZobristHash Hash = 0;
        std::array<uint64_t, Depth> Internal {};
        uint64_t Utilization = 0;
        std::mutex threadLock;

        inline std::pair<bool, uint64_t> Nodes(const ZobristHash hash, const uint8_t depth)
        {
            std::unique_lock lock(threadLock);
            return {Hash == hash && (Utilization & (1ULL << (depth - 1))), Internal[depth - 1]};
        }

        inline void Insert(const ZobristHash hash, const uint8_t depth, const uint64_t nodes)
        {
            const uint8_t  idx  = depth - 1  ;
            const uint64_t mask = 1ULL << idx;

            std::unique_lock lock(threadLock);
            if (Utilization == 0) Hash = hash;
            if (hash != Hash) return;
            Utilization |= mask;
            Internal[idx] = nodes;
        }

    };

} // Perft

#endif //STOCKDORY_PERFTENTRY_H
