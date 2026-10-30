//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_TRANSPOSITIONTABLE_H
#define STOCKDORY_TRANSPOSITIONTABLE_H

#include <bit>
#include <limits>
#include <vector>

#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Zobrist.h"

#include "../External/fastrange.h"

#include "Common.h"
#include "TunableParameter.h"

namespace StockDory
{

    constexpr size_t TranspositionTableGenerationBits = 6;

    template<typename T>
    class TranspositionTable
    {

        static_assert(sizeof(T) == sizeof(uint64_t), "Entries must be packable into 64 bits");

        constexpr static uint8_t GenerationMask = (1 << TranspositionTableGenerationBits) - 1;

        struct alignas(32) Cluster
        {

            constexpr static size_t Count = 3;

            constexpr static   size_t KeySize =                    21;
            constexpr static uint64_t KeyMask = (1ULL << KeySize) - 1;

                  Atomic<uint64_t>         Keys {};
            Array<Atomic<uint64_t>, Count> Data {};

            static uint64_t Fold(const uint64_t data)
            {
                return data ^ data >> KeySize ^ data >> KeySize * 2 ^ data >> KeySize * 3;
            }

            static bool Matches(const uint64_t keys, const uint8_t index, const uint64_t data, const ZobristHash hash)
            {
                return data && ((keys >> KeySize * index ^ Fold(data) ^ hash) & KeyMask) == 0;
            }

            uint8_t Find(const ZobristHash hash, const uint8_t generation) const
            {
                const uint64_t keys = Keys.Load(MemoryOrder::relaxed);

                uint8_t victim = 0;

                int32_t minimum = std::numeric_limits<int32_t>::max();

                for (uint8_t i = 0; i < Count; i++) {
                    const uint64_t data = Data[i].Load(MemoryOrder::relaxed);

                    if (Matches(keys, i, data, hash)) return i;

                    int32_t value = std::numeric_limits<int32_t>::min();

                    if (data) {
                        const auto    entry = std::bit_cast<T>(data);
                        const uint8_t age   = (generation - entry.Generation) & GenerationMask;

                        value = entry.Depth - age * TTReplacementAgeWeight;
                    }

                    if (value < minimum) {
                        minimum = value;

                        victim = i;
                    }
                }

                return victim;
            }

            T Load(const uint8_t index, const ZobristHash hash) const
            {
                const uint64_t data = Data[index].Load(MemoryOrder::relaxed);
                const uint64_t keys = Keys       .Load(MemoryOrder::relaxed);

                return Matches(keys, index, data, hash) ? std::bit_cast<T>(data) : T();
            }

            void Store(const uint8_t index, const ZobristHash hash, const T& value)
            {
                const uint64_t data = std::bit_cast<uint64_t>(value);
                const uint64_t key  =  (Fold(data) ^ hash) & KeyMask;

                uint64_t keys = Keys.Load(MemoryOrder::relaxed);

                keys &= ~(KeyMask << KeySize * index);
                keys |=   key     << KeySize * index ;

                Data[index].Store(data, MemoryOrder::relaxed);
                Keys       .Store(keys, MemoryOrder::relaxed);
            }

        };

        static_assert(sizeof(Cluster) == 32, "Clusters must be 32 bytes so two fit exactly into a cache line");

        using Storage = std::vector<Cluster, Memory::HugePageAllocator<Cluster>>;

        Storage Internal;

        size_t Count = 0;

        uint8_t CurrentGeneration = 0;

        public:
        explicit TranspositionTable(const size_t bytes) { Resize(bytes); }

        void Resize(const size_t bytes)
        {
            Count = bytes / sizeof(Cluster);

            Clear();
        }

        void Clear()
        {
            Internal = Storage(Count);

            CurrentGeneration = 0;
        }

        [[nodiscard]]
        size_t Size() const { return Internal.size() * Cluster::Size; }

        void BumpGeneration() { CurrentGeneration = (CurrentGeneration + 1) & GenerationMask; }

        [[nodiscard]]
        uint8_t Generation() const { return CurrentGeneration; }

        private:
        class Reference
        {

            Cluster* Internal;

            uint8_t Index     ;
            uint8_t Generation;

            ZobristHash Hash;

            public:
            explicit Reference(Cluster& cluster, const ZobristHash hash, const uint8_t generation)
            : Internal(&cluster), Index(cluster.Find(hash, generation)), Generation(generation), Hash(hash) {}

            // ReSharper disable once CppNonExplicitConversionOperator
            operator T() const { return Internal->Load(Index, Hash); }

            Reference& operator =(const T& value)
            {
                T stamped = value;

                stamped.Generation = Generation;

                Internal->Store(Index, Hash, stamped);
                return *this;
            }

        };

        public:
        Reference operator [](const ZobristHash hash)
        {
            return Reference(Internal[fastrange64(hash, Count)], hash, CurrentGeneration);
        }

        T operator [](const ZobristHash hash) const
        {
            const Cluster& cluster = Internal[fastrange64(hash, Count)];

            return cluster.Load(cluster.Find(hash, CurrentGeneration), hash);
        }

        void Prefetch(const ZobristHash hash) const
        {
            __builtin_prefetch(static_cast<const void*>(&Internal[fastrange64(hash, Count)]), 0, 3);
        }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
