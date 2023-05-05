//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UTILITYTABLE_H
#define STOCKDORY_UTILITYTABLE_H

#include "../Type/BitBoard.h"
#include "BlackMagicFactory.h"
#include "AttackTable.h"

namespace StockDory
{

    class UtilityTable
    {

        public:
            static std::array<std::array<BitBoard, 64>, 64> Between;

    };

} // StockDory

std::array<std::array<BitBoard, 64>, 64> StockDory::UtilityTable::Between = []() {
    std::array<std::array<BitBoard, 64>, 64> temp = {};

    for (Square f = A1; f != NASQ; f = Next(f)) {
        const uint8_t fH = f % 8;
        const uint8_t fV = f / 8;

        for (Square t = A1; t != NASQ; t = Next(t)) {
            temp[f][t] = BBDefault;

            if (f == t) continue;

            const uint8_t tH = t % 8;
            const uint8_t tV = t / 8;

            BitBoard occ;

            if (fH == tH || fV == tV) {
                occ = FromSquare(f) | FromSquare(t);

                const uint32_t mF = BlackMagicFactory::MagicIndex(Rook, f, occ);
                const uint32_t mT = BlackMagicFactory::MagicIndex(Rook, t, occ);

                // Rook squares:
                temp[f][t] = AttackTable::Sliding[mF] & AttackTable::Sliding[mT];

                continue;
            }

            auto absH = static_cast<int8_t>(static_cast<int8_t>(fH) - static_cast<int8_t>(tH));
            auto absV = static_cast<int8_t>(static_cast<int8_t>(fV) - static_cast<int8_t>(tV));
            if (absH < 0) absH = static_cast<int8_t>(-absH);
            if (absV < 0) absV = static_cast<int8_t>(-absV);

            if (absH != absV) continue;

            // Bishop squares:
            occ = FromSquare(f) | FromSquare(t);

            const uint32_t mF = BlackMagicFactory::MagicIndex(Bishop, f, occ);
            const uint32_t mT = BlackMagicFactory::MagicIndex(Bishop, t, occ);

            temp[f][t] = AttackTable::Sliding[mF] & AttackTable::Sliding[mT];
        }
    }

    return temp;
}();

#endif //STOCKDORY_UTILITYTABLE_H
