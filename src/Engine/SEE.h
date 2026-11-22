//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_SEE_H
#define STOCKDORY_SEE_H

#include "Board.h"

namespace StockDory
{

    class SEE
    {

        constexpr static Array<uint16_t, 7> Internal {
            82, 337, 365, 477, 1025, 30000, 0
        };

        public:
        static int32_t PieceValue(const PieceType type) { return Internal[type]; }

        static bool Accurate(const Board& board, const Move move, const int32_t threshold)
        {
            if (move.Promotion() != NAPT || move.EnPassant() || move.Castling()) return true;

            const Square from = move.From();
            const Square to   = move.  To();

            int32_t value = Internal[board[to].Type()] - threshold;
            if (value < 0) return false;

            value -= Internal[board[from].Type()];
            if (value >= 0) return true;

            const BitBoard diagonal = board.PieceBoard<White>(Bishop) | board.PieceBoard<Black>(Bishop) |
                                      board.PieceBoard<White>(Queen ) | board.PieceBoard<Black>(Queen ) ;
            const BitBoard straight = board.PieceBoard<White>(Rook  ) | board.PieceBoard<Black>(Rook  ) |
                                      board.PieceBoard<White>(Queen ) | board.PieceBoard<Black>(Queen ) ;

            BitBoard occ = board.Occupied() ^ FromSquare(from);
            BitBoard att = board.Attackers(to, occ);

            Side ctm = Opposite(board.SideToMove());
            while (true) {
                att &= occ;

                const BitBoard us = att & board[ctm];
                if (!us) break;

                PieceType type;
                for (type = Pawn; type < King; type = Next(type)) if (us & board.PieceBoard(type, ctm)) break;

                ctm = Opposite(ctm);

                value = -value - 1 - Internal[type];
                if (value >= 0) {
                    if (type == King && att & board[ctm]) ctm = Opposite(ctm);

                    break;
                }

                Clear(occ, ToSquare(us & board.PieceBoard(type, Opposite(ctm))));

                if (type == Pawn || type == Bishop || type == Queen) {
                    att |= AttackTable::Bishop(move.To(), occ) & diagonal;
                }
                if (type == Rook || type == Queen) {
                    att |= AttackTable::Rook  (move.To(), occ) & straight;
                }
            }

            return ctm != board.SideToMove();
        }

    };

} // StockDory

#endif //STOCKDORY_SEE_H
