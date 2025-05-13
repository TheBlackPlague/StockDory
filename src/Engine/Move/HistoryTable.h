//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_HISTORYTABLE_H
#define STOCKDORY_HISTORYTABLE_H

#include <array>
#include <cstdint>

#include "../../Backend/Type/Color.h"

namespace StockDory
{

    using HistoryTable = std::array<std::array<std::array<int32_t, 64>, 6>, 2>;

} // StockDory

#endif //STOCKDORY_HISTORYTABLE_H
