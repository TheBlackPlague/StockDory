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

        struct alignas(32) Cluster
        {

            std::array<Entry, ClusterSize> Entries;

        };

        static_assert(sizeof(Entry) == 8);
        static_assert(sizeof(Cluster) == 32);

        class Reference
        {

            Entry* Internal;

            public:
            explicit Reference(Entry& entry) : Internal(&entry) {}

            // ReSharper disable once CppNonExplicitConversionOperator
            operator T() const
            {
                return Internal->Load(MemoryOrder::relaxed);
            }

            Reference& operator =(const T& value)
            {
                Internal->Store(value, MemoryOrder::relaxed);

                return *this;
            }

        };

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

        Reference operator [](const ZobristHash hash)
        {
            auto& entries = Internal[fastrange64(hash, Count)].Entries;

            const auto key = CompressHash(hash);

            size_t replacement = 0;

            T previous = entries[0].Load(MemoryOrder::relaxed);

            if (previous.Type != T::EntryType::Invalid && previous.Hash == key) return Reference(entries[0]);

            for (size_t i = 1; i < ClusterSize; i++) {
                const T entry = entries[i].Load(MemoryOrder::relaxed);

                if (entry.Type != T::EntryType::Invalid && entry.Hash == key) return Reference(entries[i]);

                if (previous.Type != T::EntryType::Invalid &&
                   (   entry.Type == T::EntryType::Invalid || entry.Depth < previous.Depth)) {
                    replacement = i;
                    previous = entry;
                }
            }

            return Reference(entries[replacement]);
        }

        T operator [](const ZobristHash hash) const
        {
            const auto key = static_cast<decltype(T::Hash)>(hash);

            for (const auto& atomic : Internal[fastrange64(hash, Count)].Entries) {
                const T entry = atomic.Load(MemoryOrder::relaxed);

                if (entry.Type != T::EntryType::Invalid && entry.Hash == key) return entry;
            }

            return {};
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
