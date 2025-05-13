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

    using PV  = std::array<Move, MaxDepth>;

    struct PrincipleVariationEntry
    {

        uint8_t Ply;
        PV      PV ;

    };

    using PrincipleVariationTable = std::array<PrincipleVariationEntry, MaxDepth>;

} // StockDory

#endif //STOCKDORY_PRINCIPLEVARIATIONTABLE_H
