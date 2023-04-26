#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PREVIOUSSTATE_H
#define STOCKDORY_PREVIOUSSTATE_H

#include "PieceColor.h"
#include "Square.h"

struct PreviousState
{

    public:
        PieceColor MovedPiece                 ;
        PieceColor CapturedPiece              ;
        Piece      PromotedPiece              ;
        bool       EnPassantCapture           ;
        Square     EnPassant                  ;
        Square     CastlingFrom               ;
        Square     CastlingTo                 ;
        uint8_t    CastlingRightAndColorToMove;

        constexpr PreviousState(const PieceColor movedPiece , const PieceColor capturedPiece              ,
                                const Square     EnPassantSq, const uint8_t    castlingRightAndColorToMove)
        {
            MovedPiece                  = movedPiece;
            CapturedPiece               = capturedPiece;
            EnPassant                   = EnPassantSq;
            CastlingRightAndColorToMove = castlingRightAndColorToMove;

            EnPassantCapture = false;
            PromotedPiece    = NAP;
            CastlingFrom     = NASQ;
            CastlingTo       = NASQ;
        }

};


#endif //STOCKDORY_PREVIOUSSTATE_H

#pragma clang diagnostic pop