//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TRANSPOSITIONTABLE_H
#define STOCKDORY_TRANSPOSITIONTABLE_H

#include <vector>
#include <algorithm>
#include <execution>

#include "Type/Zobrist.h"

#include "../External/fastrange.h"

namespace StockDory
{

    template<typename T>
    class TranspositionTable
    {

        private:
            std::vector<T> Internal;
            uint64_t Count = 0;

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

            [[nodiscard]]
            inline uint64_t Size() const
            {
                return Internal.size();
            }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
