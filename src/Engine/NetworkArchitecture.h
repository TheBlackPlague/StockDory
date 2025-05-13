//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_NETWORKARCHITECTURE_H
#define STOCKDORY_NETWORKARCHITECTURE_H

#include <MantaRay/Backend/Kernel/Activation/ClippedReLU.h>
#include <MantaRay/Frontend/Architecture/Perspective.h>

// Activation Function:
constexpr auto ClippedReLU = &MantaRay::ClippedReLU<MantaRay::i16, 0, 255>::Activate;

// Architecture:
using Starshard = MantaRay::Perspective<MantaRay::i16, MantaRay::i32, ClippedReLU, 768, 256, 1, 512, 400, 255, 64>;
using Aurora    = MantaRay::Perspective<MantaRay::i16, MantaRay::i32, ClippedReLU, 768, 384, 1, 512, 400, 255, 64>;

#endif //STOCKDORY_NETWORKARCHITECTURE_H
