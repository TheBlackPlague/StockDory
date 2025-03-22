//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PRINCIPLEVARIATIONTABLE_H
#define STOCKDORY_PRINCIPLEVARIATIONTABLE_H

#include <array>
#include <cstdint>

#include "../../Backend/Type/Move.h"

#include "../EngineParameter.h"

namespace StockDory
{

    class PrincipleVariationTable
    {

        using Pv         = std::array<Move, MaxDepth>;
        using PvRelation = std::pair<uint8_t, Pv>;

        std::array<PvRelation, MaxDepth> Internal = {};

        public:
        inline void InitializePly(const uint8_t ply)
        {
            Internal[ply].first = ply;
        }

        [[nodiscard]]
        inline bool PlyInitialized(const uint8_t currentPly, const uint8_t nextPly) const
        {
            return nextPly < Internal[currentPly + 1].first;
        }

        inline void Insert(const uint8_t ply, const Move move)
        {
            Internal[ply].second[ply] = move;
        }

        inline void Copy(const uint8_t currentPly, const uint8_t nextPly)
        {
            Internal[currentPly].second[nextPly] = Internal[currentPly + 1].second[nextPly];
        }

        inline void Update(const uint8_t ply)
        {
            Internal[ply].first = Internal[ply + 1].first;
        }

        [[nodiscard]]
        inline uint8_t Count() const
        {
            return Internal[0].first;
        }

        [[nodiscard]]
        inline Move operator [](const uint8_t ply) const
        {
            return Internal[0].second[ply];
        }

    };

} // StockDory

#endif //STOCKDORY_PRINCIPLEVARIATIONTABLE_H
