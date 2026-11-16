//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_TRANSPOSITIONTABLE_H
#define STOCKDORY_TRANSPOSITIONTABLE_H

#include <algorithm>
#include <bit>
#include <cstring>
#include <vector>

#include <fastrange.h>

#include "Common.h"
#include "Memory.h"

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

            Internal = Storage(Count);
        }

        void Clear() { Clear(0, 1); }

        void Clear(const size_t part, const size_t parts)
        {
            const size_t first = Count *  part      / parts;
            const size_t last  = Count * (part + 1) / parts;

            std::memset(static_cast<void*>(Internal.data() + first), 0, (last - first) * sizeof(Entry));
        }

        [[nodiscard]]
        size_t Size() const { return Internal.size(); }

        [[nodiscard]]
        uint16_t HashFull() const
        {
            uint16_t used = 0;

            for (size_t i = 0; i < std::min<size_t>(Count, 1000); i++) {
                const Entry& entry = Internal[i];

                if (entry.Key.Load(MemoryOrder::relaxed) | entry.Data.Load(MemoryOrder::relaxed)) used++;
            }

            return used;
        }

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
