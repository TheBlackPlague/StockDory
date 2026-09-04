// Copyright (c) 2023-2026 Shaheryar Sohail
// SPDX-License-Identifier: AGPL-3.0-only

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

    ZobristHash Hash;

    constexpr PreviousState(const PieceColor  movedPiece, const PieceColor capturedPiece,
                            const Square      enPassant,  const uint8_t    castlingRightAndColorToMove,
                            const ZobristHash hash)
    {
        MovedPiece                  = movedPiece;
        CapturedPiece               = capturedPiece;
        EnPassant                   = enPassant;
        CastlingRightAndColorToMove = castlingRightAndColorToMove;

        EnPassantCapture = false;
        PromotedPiece    = NAP;
        CastlingFrom     = NASQ;
        CastlingTo       = NASQ;

        Hash = hash;
    }

};

struct PreviousStateNull
{

    Square EnPassant;

    constexpr PreviousStateNull(const Square enPassant)
    {
        EnPassant = enPassant;
    }

};

#endif //STOCKDORY_PREVIOUSSTATE_H
