//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_TRANSPOSITIONTABLE_H
#define STOCKDORY_TRANSPOSITIONTABLE_H

#include <array>
#include <vector>

#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Zobrist.h"

#include "../External/fastrange.h"

namespace StockDory
{

    template<typename T>
    class TranspositionTable
    {

        using Entry = Atomic<T>;

        static constexpr size_t ClusterSize = 4;

        public:

        struct alignas(32) Cluster
        {

            std::array<Entry, ClusterSize> Entries;

        };

        static_assert(sizeof(Entry) == 8);
        static_assert(sizeof(Cluster) == 32);

        private:
        std::vector<Cluster> Internal;

        size_t Count = 0;

        public:
        explicit TranspositionTable(const size_t bytes)
        {
            Resize(bytes);
        }

        void Resize(const size_t bytes)
        {
            Count = bytes / sizeof(Cluster);

            Clear();
        }

        void Clear()
        {
            Internal = std::vector<Cluster>(Count);
        }

        Cluster& operator [](const ZobristHash hash)
        {
            return Internal[fastrange64(hash, Count)];
        }

        const Cluster& operator [](const ZobristHash hash) const
        {
            return Internal[fastrange64(hash, Count)];
        }

        void Prefetch(const ZobristHash hash) const
        {
            __builtin_prefetch(static_cast<const void*>(&Internal[fastrange64(hash, Count)]), 0, 3);
        }

        [[nodiscard]]
        size_t Size() const
        {
            return Internal.size() * ClusterSize;
        }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
