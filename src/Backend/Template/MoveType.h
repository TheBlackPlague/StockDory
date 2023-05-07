//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef STOCKDORY_MOVETYPE_H
#define STOCKDORY_MOVETYPE_H

using MoveType = uint64_t;

constexpr MoveType STANDARD = 0x0000000F;
constexpr MoveType ZOBRIST  = 0x000000F0;
constexpr MoveType PERFT    = 0x00000F00;

constexpr MoveType NNUE     = 0x0000F000 | ZOBRIST;

#endif //STOCKDORY_MOVETYPE_H

#pragma clang diagnostic pop