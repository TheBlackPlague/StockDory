//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef STOCKDORY_MOVETYPE_H
#define STOCKDORY_MOVETYPE_H

enum MoveType : uint64_t
{

    STANDARD =                      0x000F,
    ZOBRIST  = STANDARD |           0x00F0,
    HCE      = STANDARD | ZOBRIST | 0x0F00,
    NNUE     = STANDARD | ZOBRIST | 0xF000,

};

#endif //STOCKDORY_MOVETYPE_H

#pragma clang diagnostic pop