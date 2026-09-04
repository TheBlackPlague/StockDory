// Copyright (c) 2023-2026 Shaheryar Sohail
// SPDX-License-Identifier: AGPL-3.0-only

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

        std::vector<T> Internal;
        size_t         Count = 0;

        public:
        explicit TranspositionTable(const size_t bytes)
        {
            Resize(bytes);
        }

        void Resize(const size_t bytes)
        {
            Count = bytes / sizeof(T);

            Clear();
        }

        void Clear()
        {
            Internal = std::vector<T>(Count);
        }

        T& operator [](const ZobristHash hash)
        {
            return Internal[fastrange64(hash, Count)];
        }

        const T& operator [](const ZobristHash hash) const
        {
            return Internal[fastrange64(hash, Count)];
        }

        void Prefetch(const ZobristHash hash) const
        {
            __builtin_prefetch(
                static_cast<const void*>(reinterpret_cast<const char*>(&Internal[fastrange64(hash, Count)])),
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
