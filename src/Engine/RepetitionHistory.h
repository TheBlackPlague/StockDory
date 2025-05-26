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
            uint8_t checked = 0, found = 0;
            for (uint16_t i = CurrentIndex - 1; i != 0xFFFF; i--) {
                if (checked > halfMoveCounter) break;

                if (found == 2 && Internal[i] == hash) return true;

                found += Internal[i] == hash;
                checked++;
            }

            return false;
        }

    };

} // StockDory

#endif //STOCKDORY_REPETITIONHISTORY_H
