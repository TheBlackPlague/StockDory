//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_ENGINEPARAMETER_H
#define STOCKDORY_ENGINEPARAMETER_H

#include <cstdint>

#include "EngineEntry.h"
#include "../Backend/TranspositionTable.h"

constexpr int32_t Infinity = 1000000     ;
constexpr int32_t Mate     = Infinity - 1;
constexpr int32_t Draw     = 0           ;

constexpr uint8_t MaxDepth = 128;
constexpr uint8_t MaxMove  = 218;

constexpr uint8_t NullMoveDepth            = 3  ;
constexpr uint8_t NullMoveEvaluationMargin = 180;

constexpr uint16_t AspirationBound = 3500;
constexpr uint8_t  AspirationSize  = 16  ;
constexpr uint8_t  AspirationDelta = 23  ;
constexpr uint8_t  AspirationDepth = 4   ;

constexpr uint8_t RazoringEvaluationThreshold = 150;

constexpr uint8_t LMRFullSearchThreshold = 4;
constexpr uint8_t LMRDepthThreshold      = 3;

constexpr uint8_t LMPQuietThresholdBase = 3;
constexpr uint8_t LMPDepthThreshold     = 3;

constexpr uint8_t ReverseFutilityD              = 67;
constexpr uint8_t ReverseFutilityI              = 76;
constexpr uint8_t ReverseFutilityDepthThreshold = 7 ;

constexpr uint8_t IIRDepthThreshold = 3;
constexpr uint8_t IIRDepthReduction = 1;

constexpr uint8_t FutilityDepthFactor = 150;

constexpr uint8_t CheckExtension = 1;

constexpr size_t MB = 1024 * 1024;

constexpr uint8_t ReplacementThreshold = 3;

constexpr Move NoMove = Move();

constexpr std::array<std::pair<uint16_t, uint16_t>, 5> BestMoveStabilityScale = {
        {{250, 100},
         {120, 100},
         { 90, 100},
         { 80, 100},
         { 75, 100}}
};

StockDory::TranspositionTable<StockDory::EngineEntry> TTable =
        StockDory::TranspositionTable<StockDory::EngineEntry>(16 * MB);

#endif //STOCKDORY_ENGINEPARAMETER_H
