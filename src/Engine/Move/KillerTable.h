//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_KILLERTABLE_H
#define STOCKDORY_KILLERTABLE_H

#include <cstdint>
#include <array>

#include "../../Backend/Type/Move.h"

#include "../EngineParameter.h"

namespace StockDory
{

    class KillerTable
    {

        using KillerGroup = std::array<Move, MaxDepth>;

        std::array<KillerGroup, 2> Internal = {};

        public:
        template<uint8_t Type>
        [[nodiscard]]
        inline Move Get(const uint8_t ply) const
        {
            static_assert(Type == 1 || Type == 2);

            return Internal[Type - 1][ply];
        }

        template<uint8_t Type>
        inline void Set(const uint8_t ply, const Move move)
        {
            static_assert(Type == 1 || Type == 2);

            Internal[Type - 1][ply] = move;
        }

        inline void Reorder(const uint8_t ply)
        {
            Internal[1][ply] = Internal[0][ply];
        }

    };

} // StockDory

#endif //STOCKDORY_KILLERTABLE_H
