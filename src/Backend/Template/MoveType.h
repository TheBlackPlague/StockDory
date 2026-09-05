//
// Copyright (c) 2023-2026 Shaheryar Sohail
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_MOVETYPE_H
#define STOCKDORY_MOVETYPE_H

using MoveType = uint64_t;

constexpr MoveType STANDARD = 0x0000000F;
constexpr MoveType ZOBRIST  = 0x000000F0;
constexpr MoveType PERFT    = 0x00000F00;

constexpr MoveType NNUE     = 0x0000F000;

#endif //STOCKDORY_MOVETYPE_H