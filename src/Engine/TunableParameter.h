//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TUNABLEPARAMETER_H
#define STOCKDORY_TUNABLEPARAMETER_H

#include <cstdint>

namespace StockDory
{

    constexpr uint16_t AspirationWindowFallbackBound = 3500;
    constexpr  uint8_t AspirationWindowSize          =   16;
    constexpr  uint8_t AspirationWindowSizeDelta     =   23;
    constexpr  uint8_t AspirationWindowRequiredDepth =    4;

    constexpr  uint8_t CheckExtension = 1;

    constexpr  uint8_t ReverseFutilityDisablingDepth   =  7;
    constexpr  uint8_t ReverseFutilityDepthFactor      = 67;
    constexpr  uint8_t ReverseFutilityImprovingFactor  = 76;

} // StockDory

#endif //STOCKDORY_TUNABLEPARAMETER_H
