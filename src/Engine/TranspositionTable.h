//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
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

        static_assert(std::is_trivially_copyable_v<T>, "Transposition table entries must be trivially copyable");
        static_assert(std::atomic<T>::is_always_lock_free, "Transposition table entries must use lock-free atomics");

        using Atomic = std::atomic<T>;

        class Reference
        {

            Atomic* Internal;

            public:
            explicit Reference(Atomic& entry) : Internal(&entry) {}

            // ReSharper disable once CppNonExplicitConversionOperator
            operator T() const
            {
                return Internal->load(std::memory_order::relaxed);
            }

            Reference& operator =(const T& value)
            {
                Internal->store(value, std::memory_order::relaxed);

                return *this;
            }

        };

        std::vector<Atomic> Internal;
        size_t              Count = 0;

        public:
        explicit TranspositionTable(const size_t bytes)
        {
            Resize(bytes);
        }

        void Resize(const size_t bytes)
        {
            Count = bytes / sizeof(Atomic);

            Clear();
        }

        void Clear()
        {
            Internal = std::vector<Atomic>(Count);

            for (Atomic& entry : Internal) entry.store(T {}, std::memory_order::relaxed);
        }

        Reference operator [](const ZobristHash hash)
        {
            return Reference(Internal[fastrange64(hash, Count)]);
        }

        T operator [](const ZobristHash hash) const
        {
            return Internal[fastrange64(hash, Count)].load(std::memory_order::relaxed);
        }

        void Prefetch(const ZobristHash hash) const
        {
            __builtin_prefetch(
                static_cast<const void*>(&Internal[fastrange64(hash, Count)]),
                0,
                3
            );
        }

        [[nodiscard]]
        size_t Size() const
        {
            return Internal.size();
        }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
