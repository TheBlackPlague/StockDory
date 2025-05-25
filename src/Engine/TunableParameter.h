//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TUNABLEPARAMETER_H
#define STOCKDORY_TUNABLEPARAMETER_H

#include <cstdint>

namespace StockDory
{

    constexpr  uint8_t TimeBasePartitionNumerator        =  1;
    constexpr  uint8_t TimeBasePartitionDenominator      = 20;
    constexpr  uint8_t TimeIncrementPartitionNumerator   =  3;
    constexpr  uint8_t TimeIncrementPartitionDenominator =  4;
    constexpr  uint8_t TimeProcessingOverhead            = 10;

    constexpr Array<uint16_t, 5> BestMoveStabilityOptimizationFactor {
        250, 180, 120, 99, 97
    };

    constexpr uint16_t AspirationWindowFallbackBound = 3500;
    constexpr  uint8_t AspirationWindowSize          =   16;
    constexpr  uint8_t AspirationWindowSizeDelta     =   23;
    constexpr  uint8_t AspirationWindowMinimumDepth  =    5;

    constexpr  uint8_t CheckExtension = 1;

    constexpr  uint8_t ReverseFutilityMaximumDepth     =  7;
    constexpr  uint8_t ReverseFutilityDepthFactor      = 67;
    constexpr  uint8_t ReverseFutilityImprovingFactor  = 76;

    constexpr  uint8_t RazoringDepth            =   1;
    constexpr  uint8_t RazoringEvaluationMargin = 150;

    constexpr  uint8_t NullMoveMinimumDepth     =   3;
    constexpr  uint8_t NullMoveMinimumReduction =   3;
    constexpr  uint8_t NullMoveDepthFactor      =   3;
    constexpr  uint8_t NullMoveEvaluationFactor = 180;

    constexpr  uint8_t IIRMinimumDepth   = 4;
    constexpr  uint8_t IIRDepthReduction = 1;

    constexpr  uint8_t LMPMaximumDepth  = 3;
    constexpr  uint8_t LMPLastQuietBase = 3;

    constexpr  uint8_t LMRMinimumDepth = 3;
    constexpr  uint8_t LMRMinimumMoves = 3;

    constexpr  uint8_t FutilityDepthFactor = 150;

    constexpr  uint8_t TTReplacementDepthMargin = 3;

} // StockDory

#endif //STOCKDORY_TUNABLEPARAMETER_H
