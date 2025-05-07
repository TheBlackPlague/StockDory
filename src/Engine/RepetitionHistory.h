//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_REPETITIONHISTORY_H
#define STOCKDORY_REPETITIONHISTORY_H

#include <cstdint>
#include <array>

#include "../Backend/Type/Zobrist.h"

namespace StockDory
{

    class RepetitionHistory
    {

        std::array<ZobristHash, 4096> Internal = {};

        uint16_t CurrentIndex = 0;

        public:
        explicit RepetitionHistory(const ZobristHash hash)
        {
            Push(hash);
        }

        void Push(const ZobristHash hash)
        {
            Internal[CurrentIndex++] = hash;
        }

        void Pull()
        {
            CurrentIndex--;
        }

        [[nodiscard]]
        bool Found(const ZobristHash hash, const uint8_t halfMoveCounter) const
        {
            uint8_t count = 0;
            for (uint16_t i = CurrentIndex - 1; i != 0xFFFF; i--) {
                if (i < CurrentIndex - 1 - halfMoveCounter) return false;

                if (Internal[i] == hash) {
                    count++;
                    if (count > 2) return true;
                }
            }

            return false;
        }

    };

} // StockDory

#endif //STOCKDORY_REPETITIONHISTORY_H
