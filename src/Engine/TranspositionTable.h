//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_TRANSPOSITIONTABLE_H
#define STOCKDORY_TRANSPOSITIONTABLE_H

#include <bit>
#include <vector>

#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Zobrist.h"

#include "../External/fastrange.h"

#include "Common.h"

namespace StockDory
{

    template<typename T>
    class TranspositionTable
    {

        static_assert(sizeof(T) == sizeof(uint64_t), "Entries must be packable into 64 bits");

        struct Entry
        {

            Atomic<uint64_t> Key  {};
            Atomic<uint64_t> Data {};

            T Load(const ZobristHash hash) const
            {
                const uint64_t data = Data.Load(MemoryOrder::relaxed);
                const uint64_t key  = Key .Load(MemoryOrder::relaxed);

                return (key ^ data) == hash ? std::bit_cast<T>(data) : T();
            }

            void Store(const ZobristHash hash, const T& value)
            {
                const auto data = std::bit_cast<uint64_t>(value);

                Data.Store(data       , MemoryOrder::relaxed);
                Key .Store(data ^ hash, MemoryOrder::relaxed);
            }

        };

        using Storage = std::vector<Entry, Memory::HugePageAllocator<Entry>>;

        Storage Internal;

        size_t Count = 0;

        public:
        explicit TranspositionTable(const size_t bytes) { Resize(bytes); }

        void Resize(const size_t bytes)
        {
            Count = bytes / sizeof(Entry);

            Clear();
        }

        void Clear() { Internal = Storage(Count); }

        [[nodiscard]]
        size_t Size() const { return Internal.size(); }

        private:
        class Reference
        {

            Entry*      Internal;
            ZobristHash Hash    ;

            public:
            explicit Reference(Entry& entry, const ZobristHash hash) : Internal(&entry), Hash(hash) {}

            // ReSharper disable once CppNonExplicitConversionOperator
            operator T() const { return Internal->Load(Hash); }

            Reference& operator =(const T& value) { Internal->Store(Hash, value); return *this; }

        };

        public:
        Reference operator [](const ZobristHash hash) { return Reference(Internal[fastrange64(hash, Count)], hash); }

        T operator [](const ZobristHash hash) const { return Internal[fastrange64(hash, Count)].Load(hash); }

        void Prefetch(const ZobristHash hash) const
        {
            __builtin_prefetch(static_cast<const void*>(&Internal[fastrange64(hash, Count)]), 0, 3);
        }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
