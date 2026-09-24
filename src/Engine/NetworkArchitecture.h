//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_NETWORKARCHITECTURE_H
#define STOCKDORY_NETWORKARCHITECTURE_H

#include <MantaRay/MantaRay.h>

template<MantaRay::s00 HiddenSize>
using MantaRayArchitecture = MantaRay::Network<
    MantaRay::Quantization<255, 64, 400>,
    MantaRay::Mirror<
        MantaRay::Accumulate<
            MantaRay::Layer<768, HiddenSize, MantaRay::ClippedReLU<0, 1>>
        >
    >,
    MantaRay::Concat,
    MantaRay::Layer<HiddenSize * 2, 1>
>;

using StarshardArchitecture = MantaRayArchitecture<256>;
using    AuroraArchitecture = MantaRayArchitecture<384>;

using Starshard = MantaRay::Runtime::Network<StarshardArchitecture>;
using    Aurora = MantaRay::Runtime::Network<   AuroraArchitecture>;

// Accumulator Stack:
constexpr size_t AccumulatorStackSize = StockDory::MaxDepth * 4;

using StarshardStack = MantaRay::Runtime::AccumulatorStack<StarshardArchitecture, AccumulatorStackSize>;
using    AuroraStack = MantaRay::Runtime::AccumulatorStack<   AuroraArchitecture, AccumulatorStackSize>;

#endif //STOCKDORY_NETWORKARCHITECTURE_H
