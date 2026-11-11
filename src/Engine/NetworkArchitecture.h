//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_NETWORKARCHITECTURE_H
#define STOCKDORY_NETWORKARCHITECTURE_H

#include <MantaRay/MantaRay.h>

// Architecture:
namespace Networks
{

    template<size_t Hidden, bool SquaredActivation = true>
    constexpr auto PerspectiveBase768()
    {
        using namespace MantaRay;

        auto [stm, ntm] = Input<Chess768>();

        const auto l0 = Affine(768, Hidden, Weights<i16>(255, Clip(1.98)), Biases<i16>(255, Clip(1.98)));
        const auto l1 = SquaredActivation ?
            Affine(2 * Hidden, 1, Weights<i16>(64, Clip(1.98)), Biases<i16>(255 * 64), DivideBy(255)) :
            Affine(2 * Hidden, 1, Weights<i16>(64, Clip(1.98)), Biases<i16>(255 * 64)               ) ;

        const auto yStm = SquaredActivation ? SquaredClippedReLU(l0(stm)) : ClippedReLU(l0(stm));
        const auto yNtm = SquaredActivation ? SquaredClippedReLU(l0(ntm)) : ClippedReLU(l0(ntm));

        const auto yConcat = Concat(yStm, yNtm);

        const auto y = l1(yConcat);

        return Output(y, 400);
    }

    constexpr auto Starshard() { return PerspectiveBase768<256, false>(); }
    constexpr auto Aurora   () { return PerspectiveBase768<384, false>(); }
    constexpr auto Flare    () { return PerspectiveBase768<768, false>(); }

    constexpr auto Corona () { return PerspectiveBase768< 768>(); }
    constexpr auto Eclipse() { return PerspectiveBase768<1536>(); }

} // Networks

// Network:
using Starshard = MantaRay::Network<Networks::Starshard>;
using    Aurora = MantaRay::Network<Networks::Aurora   >;
using     Flare = MantaRay::Network<Networks::Flare    >;
using    Corona = MantaRay::Network<Networks::Corona   >;
using   Eclipse = MantaRay::Network<Networks::Eclipse  >;

// Accumulator Stack:
constexpr size_t AccumulatorStackSize = StockDory::MaxDepth + 1;

// Defaults to lazy updates
using StarshardStack = MantaRay::AccumulatorStack<Networks::Starshard, AccumulatorStackSize>;
using    AuroraStack = MantaRay::AccumulatorStack<Networks::Aurora   , AccumulatorStackSize>;
using     FlareStack = MantaRay::AccumulatorStack<Networks::Flare    , AccumulatorStackSize>;
using    CoronaStack = MantaRay::AccumulatorStack<Networks::Corona   , AccumulatorStackSize>;
using   EclipseStack = MantaRay::AccumulatorStack<Networks::Eclipse  , AccumulatorStackSize>;

#endif //STOCKDORY_NETWORKARCHITECTURE_H
