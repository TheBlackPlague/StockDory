//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TRANSPOSITIONTABLE_H
#define STOCKDORY_TRANSPOSITIONTABLE_H

#include <vector>

#ifdef __x86_64__
#include <xmmintrin.h>
#endif

#include "Type/Zobrist.h"

#include "../External/fastrange.h"

namespace StockDory
{

    template<typename T>
    class TranspositionTable
    {

        std::vector<T> Internal;
        uint64_t       Count = 0;

        public:
        explicit TranspositionTable(const uint64_t bytes)
        {
            Resize(bytes);
        }

        void Resize(const uint64_t bytes)
        {
            Count = bytes / sizeof(T);

            Clear();
        }

        void Clear()
        {
            Internal = std::vector<T>(Count);
        }

        inline T& operator [](const ZobristHash hash)
        {
            return Internal[fastrange64(hash, Count)];
        }

        inline const T& operator [](const ZobristHash hash) const
        {
            return Internal[fastrange64(hash, Count)];
        }

        inline void Prefetch(const ZobristHash hash) const
        {
#ifdef __x86_64__
            _mm_prefetch(reinterpret_cast<const char*>(&Internal[fastrange64(hash, Count)]), _MM_HINT_T0);
#else
#ifdef __aarch64__
            __builtin_prefetch(reinterpret_cast<const char*>(&Internal[fastrange64(hash, Count)]), 0, 3);
#endif
#endif
        }

        [[nodiscard]]
        inline uint64_t Size() const
        {
            return Internal.size();
        }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
