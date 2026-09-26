//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_TUNABLEPARAMETER_H
#define STOCKDORY_TUNABLEPARAMETER_H

#include <cstdint>

#ifdef BUILD_TUNING
#include <type_traits>
#include <vector>
#endif

namespace StockDory
{

#ifdef BUILD_TUNING

    struct TunableParameter
    {

        const char* Name;

        int32_t Default;
        int32_t     Min;
        int32_t     Max;

        void (*Set)(int32_t);

        static std::vector<TunableParameter>& All()
        {
            static std::vector<TunableParameter> parameters;

            return parameters;
        }

        template<typename T>
        static constexpr int32_t Encode(const T value)
        {
            if (std::is_floating_point_v<T>) {
                const double scaled = static_cast<double>(value) * 1000;

                return static_cast<int32_t>(scaled + (scaled < 0 ? -0.5 : 0.5));
            } else return static_cast<int32_t>(value);
        }

        template<auto& Variable>
        static bool Register(
            const char* name,
            const std::remove_reference_t<decltype(Variable)> value,
            const std::remove_reference_t<decltype(Variable)>  min ,
            const std::remove_reference_t<decltype(Variable)>  max )
        {
            using T = std::remove_reference_t<decltype(Variable)>;

            All().push_back({
                name,
                Encode(value),
                Encode( min ),
                Encode( max ),
                [](const int32_t _value)
                {
                    if (std::is_floating_point_v<T>) Variable = static_cast<T>(_value / 1000.0);
                    else                             Variable = static_cast<T>(_value         );
                }
            });

            return true;
        }

    };

#define PARAMETER(Type, Name, Value, Min, Max) \
    inline constinit Type Name = Value; \
    [[maybe_unused]] inline const bool Name##Registered = TunableParameter::Register<Name>(#Name, Value, Min, Max)

#else

#define PARAMETER(Type, Name, Value, Min, Max) constexpr Type Name = Value

#endif

    PARAMETER(uint8_t, TimeDefaultMovesToGo             , 24, 12, 48);
    PARAMETER(uint8_t, TimeIncrementPartitionNumerator  ,  3,  1,  4);
    PARAMETER(uint8_t, TimeIncrementPartitionDenominator,  4,  4,  8);
    PARAMETER(uint8_t, TimeProcessingOverhead           , 10,  0, 50);
    PARAMETER(uint8_t, TimeHardLimitMultiplier          ,  5,  2,  8);

    PARAMETER(uint8_t, TimeSingleMovePartitionNumerator  ,  1,  0,  2);
    PARAMETER(uint8_t, TimeSingleMovePartitionDenominator, 20, 10, 40);

    PARAMETER(uint8_t, TimeManagementMinimumDepth, 10    , 4    , 20    );
    PARAMETER(double , TimeNodeBase              ,  2.025, 1.500,  3.000);
    PARAMETER(double , TimeNodeEffortWeight      ,  1.350, 0.500,  1.500);

    PARAMETER(uint16_t, AspirationWindowFallbackBound, 3500, 2000, 6000);
    PARAMETER(uint8_t , AspirationWindowMargin       ,   16,    8,   64);
    PARAMETER(uint8_t , AspirationWindowMarginDelta  ,   23,    8,   64);
    PARAMETER(uint8_t , AspirationWindowMinimumDepth ,    5,    3,    8);

    PARAMETER(uint8_t, CheckExtension, 1, 0, 1);

    PARAMETER(uint8_t, ReverseFutilityMaximumDepth   ,  7,  3,  10);
    PARAMETER(uint8_t, ReverseFutilityDepthFactor    , 80, 40, 160);
    PARAMETER(uint8_t, ReverseFutilityImprovingFactor, 60,  0, 120);

    PARAMETER(uint8_t, RazoringDepth           ,   1,  1,   3);
    PARAMETER(uint8_t, RazoringEvaluationMargin, 150, 75, 255);

    PARAMETER(uint8_t, NullMoveMinimumDepth    ,   3,  2,   8);
    PARAMETER(uint8_t, NullMoveMinimumReduction,   3,  1,   6);
    PARAMETER(uint8_t, NullMoveDepthFactor     ,   3,  2,   6);
    PARAMETER(uint8_t, NullMoveEvaluationFactor, 180, 90, 255);

    PARAMETER(uint8_t, IIRMinimumDepth  , 4, 3, 8);
    PARAMETER(uint8_t, IIRDepthReduction, 1, 0, 2);

    PARAMETER(uint8_t, LMPMaximumDepth , 3, 1,  8);
    PARAMETER(uint8_t, LMPLastQuietBase, 3, 1, 16);

    PARAMETER(uint8_t , LMRMinimumDepth             ,    3,   2,    8);
    PARAMETER(uint8_t , LMRMinimumMoves             ,    3,   2,    8);
    PARAMETER(uint16_t, LMRNotPVBonus               , 1024,   0, 2048);
    PARAMETER(uint16_t, LMRTTMoveBonus              , 1024,   0, 2048);
    PARAMETER(uint16_t, LMRNotImprovingBonus        , 1024,   0, 2048);
    PARAMETER(uint16_t, LMRGaveCheckPenalty         , 1024,   0, 2048);
    PARAMETER(uint16_t, LMRHistoryWeight            , 1024, 512, 2048);
    PARAMETER(uint16_t, LMRHistoryPartition         ,    2,   1,    4);
    PARAMETER(uint16_t, LMRContinuationHistoryWeight, 1024,   0, 2048);

    PARAMETER(uint8_t, FutilityDepthFactor, 150, 75, 255);

    PARAMETER(uint16_t, HistoryMultiplier, 300, 150, 450);
    PARAMETER(uint16_t, HistoryShiftDown , 250,   0, 500);

    PARAMETER(uint16_t, MaterialScalingWeightPawn        ,    0,    0,   100);
    PARAMETER(uint16_t, MaterialScalingWeightKnight      ,  308,  150,   500);
    PARAMETER(uint16_t, MaterialScalingWeightBishop      ,  346,  150,   550);
    PARAMETER(uint16_t, MaterialScalingWeightRook        ,  521,  300,   800);
    PARAMETER(uint16_t, MaterialScalingWeightQueen       ,  994,  600,  1200);

    PARAMETER(uint8_t, TTReplacementDepthMargin, 3, 0, 6);

#undef PARAMETER

}

#endif
