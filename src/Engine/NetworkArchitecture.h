//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_NETWORKARCHITECTURE_H
#define STOCKDORY_NETWORKARCHITECTURE_H

#include <MantaRay/MantaRay.h>

// Architecture:
template<MantaRay::s00 HiddenSize>
using PerspectiveArchitecture = MantaRay::Architecture<
    MantaRay::FeatureTransformer<MantaRay::Chess768, HiddenSize,
        MantaRay::Weights<MantaRay::i16, 255, MantaRay::Clip<-1.98, 1.98>>,
        MantaRay::Biases <MantaRay::i16, 255, MantaRay::Clip<-1.98, 1.98>>
    >,
    MantaRay::ClippedReLU<>, // Defaults to [0, 1]
    MantaRay::Concat,
    MantaRay::Affine<HiddenSize * 2, 1,
        MantaRay::Weights<MantaRay::i16,       64, MantaRay::Clip<-1.98, 1.98>>,
        MantaRay::Biases <MantaRay::i16, 255 * 64                             >
    >,
    MantaRay::Output<400>
>;

using StarshardArchitecture = PerspectiveArchitecture<256>;
using    AuroraArchitecture = PerspectiveArchitecture<384>;
using     FlareArchitecture = PerspectiveArchitecture<768>;

// Network:
using Starshard = MantaRay::Network<StarshardArchitecture>;
using    Aurora = MantaRay::Network<   AuroraArchitecture>;
using     Flare = MantaRay::Network<    FlareArchitecture>;

// Accumulator Stack:
constexpr size_t AccumulatorStackSize = StockDory::MaxDepth + 1;

// Defaults to lazy updates
template<typename Architecture>
using NetworkStack = MantaRay::AccumulatorStack<Architecture, AccumulatorStackSize>;

using StarshardStack = NetworkStack<StarshardArchitecture>;
using    AuroraStack = NetworkStack<   AuroraArchitecture>;
using     FlareStack = NetworkStack<    FlareArchitecture>;

#endif //STOCKDORY_NETWORKARCHITECTURE_H
