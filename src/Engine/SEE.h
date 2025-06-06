//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEE_H
#define STOCKDORY_SEE_H

#include "../Backend/Board.h"
#include "../Backend/Type/Move.h"

#include "Common.h"

namespace StockDory
{

    class SEE
    {

        constexpr static Array<uint16_t, 7> Internal {
            82, 337, 365, 477, 1025, 30000, 0
        };

        public:
        static bool Accurate(const Board& board, const Move move, const int32_t threshold)
        {
            if (move.Promotion() != NAP) return true;

            const Square from = move.From();
            const Square to   = move.  To();

            if (board[from].Piece() == Pawn &&  to == board.EnPassantSquare()                ) return true;
            if (board[from].Piece() == King && (to == C1 || to == C8 || to == G1 || to == G8)) return true;

            int32_t value = Internal[board[to].Piece()] - threshold;
            if (value < 0) return false;

            value -= Internal[board[from].Piece()];
            if (value >= 0) return true;

            const BitBoard diagonal = board.PieceBoard<White>(Bishop) | board.PieceBoard<Black>(Bishop) |
                                      board.PieceBoard<White>(Queen ) | board.PieceBoard<Black>(Queen ) ;
            const BitBoard straight = board.PieceBoard<White>(Rook  ) | board.PieceBoard<Black>(Rook  ) |
                                      board.PieceBoard<White>(Queen ) | board.PieceBoard<Black>(Queen ) ;

            BitBoard occ = ~board[NAC] ^ FromSquare(from);
            BitBoard att = board.SquareAttackers(to, occ);

            Color ctm = Opposite(board.ColorToMove());
            while (true) {
                att &= occ;

                const BitBoard us = att & board[ctm];
                if (!us) break;

                Piece piece;
                for (piece = Pawn; piece < King; piece = Next(piece)) if (us & board.PieceBoard(piece, ctm)) break;

                ctm = Opposite(ctm);

                value = -value - 1 - Internal[piece];
                if (value >= 0) {
                    if (piece == King && att & board[ctm]) ctm = Opposite(ctm);

                    break;
                }

                Set<false>(occ, ToSquare(us & board.PieceBoard(piece, Opposite(ctm))));

                if (piece == Pawn || piece == Bishop || piece == Queen) {
                    const uint32_t idx = BlackMagicFactory::MagicIndex(Bishop, move.To(), occ);
                    att |= AttackTable::Sliding[idx] & diagonal;
                }
                if (piece == Rook || piece == Queen) {
                    const uint32_t idx = BlackMagicFactory::MagicIndex(Rook, move.To(), occ);
                    att |= AttackTable::Sliding[idx] & straight;
                }
            }

            return ctm != board.ColorToMove();
        }

    };

} // StockDory

#endif //STOCKDORY_SEE_H
