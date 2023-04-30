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

namespace StockDory
{

    template<typename T>
    class TranspositionTable
    {

        private:
            std::vector<T> Internal;
            uint64_t IndexMask = 0;

        public:
            explicit constexpr TranspositionTable(const uint64_t bytes)
            {
                Resize(bytes);
            }

            constexpr void Resize(const uint64_t bytes)
            {
                for (uint64_t i = 0x1; (i + 1) * sizeof(T) <= bytes; i = i << 1 | 0x1) {
                    IndexMask = i;
                }

                const uint64_t size = IndexMask + 1;

                Internal = std::vector<T>(size);

                // std::fill(std::execution::par, Internal.begin(), Internal.end(), T());
            }

            constexpr inline T& operator [](const ZobristHash index)
            {
                return Internal[index & IndexMask];
            }

    };

} // StockDory

#endif //STOCKDORY_TRANSPOSITIONTABLE_H
