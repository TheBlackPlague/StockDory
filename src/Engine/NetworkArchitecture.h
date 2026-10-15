//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_NETWORKARCHITECTURE_H
#define STOCKDORY_NETWORKARCHITECTURE_H

#include <MantaRay/Backend/Kernel/Activation/ClippedReLU.h>
#include <MantaRay/Frontend/Architecture/Perspective.h>
#include <MantaRay/Frontend/Architecture/Common/AccumulatorStack.h>

// Activation Function:
constexpr auto ClippedReLU = &MantaRay::ClippedReLU<MantaRay::i16, 0, 255>::Activate;

// Architecture:
using Starshard = MantaRay::Perspective<
    MantaRay::i16, MantaRay::i32, ClippedReLU, 768, 256, 1, 397, 255, 64
>;
using Aurora    = MantaRay::Perspective<
    MantaRay::i16, MantaRay::i32, ClippedReLU, 768, 384, 1, 397, 255, 64
>;
using Flare     = MantaRay::Perspective<
    MantaRay::i16, MantaRay::i32, ClippedReLU, 768, 768, 1, 397, 255, 64
>;

// Accumulator Stack:
constexpr size_t AccumulatorStackSize = StockDory::MaxDepth + 1;

using StarshardStack = MantaRay::AccumulatorStack<MantaRay::i16, 256, AccumulatorStackSize>;
using    AuroraStack = MantaRay::AccumulatorStack<MantaRay::i16, 384, AccumulatorStackSize>;
using     FlareStack = MantaRay::AccumulatorStack<MantaRay::i16, 768, AccumulatorStackSize>;

#endif //STOCKDORY_NETWORKARCHITECTURE_H
