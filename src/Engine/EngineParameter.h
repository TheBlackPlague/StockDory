//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_ENGINEPARAMETER_H
#define STOCKDORY_ENGINEPARAMETER_H

#include "SearchState.h"
#include "TranspositionTable.h"

constexpr uint8_t NullMoveDepth            = 3  ;
constexpr uint8_t NullMoveEvaluationMargin = 180;

constexpr uint16_t AspirationBound = 3500;
constexpr uint8_t  AspirationSize  = 16  ;
constexpr uint8_t  AspirationDelta = 23  ;
constexpr uint8_t  AspirationDepth = 4   ;

constexpr uint8_t RazoringAlphaThreshold = 2000;
constexpr uint8_t RazoringDepthFactor    = 350 ;
constexpr uint8_t RazoringDepthShift     = 0   ;

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

constexpr int16_t HistoryLimit = 16384;

constexpr size_t MB = 1024 * 1024;

constexpr uint8_t ReplacementThreshold = 3;

constexpr auto NoMove = Move();

constexpr std::array<std::pair<uint16_t, uint16_t>, 5> BestMoveStabilityOptimisationFactor = {{
    {250, 100},
    {180, 100},
    {120, 100},
    { 99, 100},
    { 97, 100}
}};

inline auto TTable = StockDory::TranspositionTable<StockDory::SearchState>(16 * MB);

#endif //STOCKDORY_ENGINEPARAMETER_H
