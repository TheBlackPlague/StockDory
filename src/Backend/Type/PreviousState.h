//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_PREVIOUSSTATE_H
#define STOCKDORY_PREVIOUSSTATE_H

#include "PieceColor.h"
#include "Square.h"
#include "Zobrist.h"

struct PreviousState
{

    PieceColor MovedPiece                 ;
    PieceColor CapturedPiece              ;
    Piece      PromotedPiece              ;
    bool       EnPassantCapture           ;
    Square     EnPassant                  ;
    Square     CastlingFrom               ;
    Square     CastlingTo                 ;
    uint8_t    CastlingRightAndColorToMove;

          ZobristHash     Hash     ;
          ZobristHash     HashMinor;
    Array<ZobristHash, 2> HashMajor;

};

struct PreviousStateNull
{

    Square EnPassant;

};

#endif //STOCKDORY_PREVIOUSSTATE_H
