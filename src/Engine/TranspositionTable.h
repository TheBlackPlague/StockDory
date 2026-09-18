//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_TRANSPOSITIONTABLE_H
#define STOCKDORY_TRANSPOSITIONTABLE_H

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

        std::vector<Entry> Internal;

        size_t Count = 0;

        public:
        explicit TranspositionTable(const size_t bytes)
        {
            Resize(bytes);
        }

        void Resize(const size_t bytes)
        {
            Count = bytes / sizeof(Entry);

            Clear();
        }

        void Clear()
        {
            Internal = std::vector<Entry>(Count);
        }

        Reference operator [](const ZobristHash hash)
        {
            return Reference(Internal[fastrange64(hash, Count)]);
        }

        T operator [](const ZobristHash hash) const
        {
            return Internal[fastrange64(hash, Count)].Load(MemoryOrder::relaxed);
        }

        void Prefetch(const ZobristHash hash) const
        {
            __builtin_prefetch(static_cast<const void*>(&Internal[fastrange64(hash, Count)]), 0, 3);
        }

        [[nodiscard]]
        size_t Size() const
        {
            return Internal.size();
        }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
