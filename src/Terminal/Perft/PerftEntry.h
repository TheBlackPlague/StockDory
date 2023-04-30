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

        [[nodiscard]]
        inline std::pair<bool, uint64_t> Nodes(const ZobristHash hash, const uint8_t depth)
        {
            std::unique_lock lock(threadLock);
            if (hash == Hash) {
                const uint8_t idx = depth - 1;
                return {Utilization & (1ULL << idx), Internal[idx]};
            }

            return {false, 0};
        }

        inline void Insert(const ZobristHash hash, const uint8_t depth, const uint64_t nodes)
        {
            std::unique_lock lock(threadLock);
            const uint8_t idx = depth - 1;
            if (Utilization) {
                if (hash == Hash) {
                    Internal[idx] = nodes;
                    Utilization |= (1ULL << (idx));
                }
            } else {
                Hash = hash;
                Internal[idx] = nodes;
                Utilization |= (1ULL << (idx));
            }
        }

    };

} // Perft

#endif //STOCKDORY_PERFTENTRY_H
