//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_NETWORKARCHITECTURE_H
#define STOCKDORY_NETWORKARCHITECTURE_H

#include <MantaRay/Perspective/PerspectiveNNUE.h>
#include <MantaRay/Activation/ClippedReLU.h>

// Activation Function:
using CRelu000255 = MantaRay::ClippedReLU<int16_t, 0, 255>;

// Architecture:
using Starshard = MantaRay::PerspectiveNetwork<int16_t, int32_t, CRelu000255, 768, 256, 1, 512, 400, 255, 64>;
using Aurora    = MantaRay::PerspectiveNetwork<int16_t, int32_t, CRelu000255, 768, 384, 1, 512, 400, 255, 64>;

#endif //STOCKDORY_NETWORKARCHITECTURE_H
