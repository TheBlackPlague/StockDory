//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_KILLERTABLE_H
#define STOCKDORY_KILLERTABLE_H

#include <array>
#include <cstdint>

#include "../../Backend/Type/Move.h"

#include "../EngineParameter.h"

namespace StockDory
{

    using KillerGroup = std::array<Move, MaxDepth>;
    using KillerTable = std::array<KillerGroup, 2>;

} // StockDory

#endif //STOCKDORY_KILLERTABLE_H
