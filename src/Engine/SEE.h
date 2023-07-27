//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEE_H
#define STOCKDORY_SEE_H

#include <cstdint>
#include <array>

#include "../Backend/Board.h"
#include "../Backend/Type/Move.h"

namespace StockDory
{

    class SEE
    {

        private:
            constexpr static std::array<uint16_t, 7> Internal = {
                    82, 337, 365, 477, 1025, 30000, 0
            };

            static inline bool Unchecked(const Board& board, const Move move)
            {
                if (move.Promotion() != NAP) return true;

                const Piece from = board[move.From()].Piece();
                if (from == Pawn && move.To() == board.EnPassantSquare()) return true;

                return from == King && (move.To() == C1 || move.To() == C8 || move.To() == G1 || move.To() == G8);
            }

        public:
            static inline int32_t Approximate(const Board& board, const Move move)
            {
                Piece from = board[move.From()].Piece();
                Piece to   = board[move.  To()].Piece();

                if (from == Pawn && move.To() == board.EnPassantSquare()) to = Pawn;

                int32_t value = Internal[to];

                if (move.Promotion() != NAP) value += Internal[move.Promotion()] - Internal[Pawn];

                return value - Internal[from];
            }

            static inline bool Accurate(const Board& board, const Move move, const int32_t threshold)
            {
                if (Unchecked(board, move)) return true;

                const Square from = move.From();
                const Square to   = move.To  ();

                int32_t value = Internal[board[to].Piece()] - threshold;
                if (value <  0) return false;

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
                    for ( piece = Pawn; piece < King; piece = Next(piece))
                        if (us & board.PieceBoard(piece, ctm)) break;

                    ctm = Opposite(ctm);

                    value = -value - 1 - Internal[piece];
                    if (value >= 0) {
                        if (piece == King && (att & board[ctm])) ctm = Opposite(ctm);

                        break;
                    }

                    Set<false>(occ, ToSquare(us & board.PieceBoard(piece, Opposite(ctm))));

                    if (piece == Pawn || piece == Bishop || piece == Queen) {
                        const uint32_t idx = BlackMagicFactory::MagicIndex(Bishop, move.To(), occ);
                        att |= AttackTable::Sliding[idx] & diagonal;
                    }
                    if (piece == Rook || piece == Queen) {
                        const uint32_t idx = BlackMagicFactory::MagicIndex(Rook  , move.To(), occ);
                        att |= AttackTable::Sliding[idx] & straight;
                    }
                }

                return ctm != board.ColorToMove();
            }

    };

} // StockDory

#endif //STOCKDORY_SEE_H
