//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVELIST_H
#define STOCKDORY_MOVELIST_H

#include <array>

#include "../Type/BitBoard.h"
#include "../Type/Piece.h"
#include "../Type/Color.h"
#include "../Type/Square.h"
#include "../Type/Move.h"
#include "../Type/PinBitBoard.h"
#include "../Type/CheckBitBoard.h"

#include "../Board.h"

#include "AttackTable.h"
#include "BlackMagicFactory.h"

namespace StockDory
{

    template<Piece Piece, Color Color>
    class MoveList
    {

        private:
            BitBoard InternalContainer;

            constexpr static BitBoard WhiteQueenCastlePath = 0x0EULL                   ;
            constexpr static BitBoard WhiteKingCastlePath  = 0x60ULL                   ;
            constexpr static BitBoard BlackQueenCastlePath = WhiteQueenCastlePath << 56;
            constexpr static BitBoard BlackKingCastlePath  = WhiteKingCastlePath  << 56;

            constexpr static BitBoard QueenCastlePathMask  = 0x0C0000000000000CULL;

        public:
            constexpr static bool Promotion(const Square sq)
            {
                if (Piece != Piece::Pawn) return false;

                return (Color == White && sq > H6) || (Color == Black && sq < A3);
            }

            constexpr MoveList(const Board& board, const Square sq, const PinBitBoard& pin, const CheckBitBoard& check)
            {
                InternalContainer = BBDefault;

                if (Piece == Piece::Pawn  ) Pawn  (board, sq, pin, check);
                if (Piece == Piece::Knight) Knight(board, sq, pin, check);
                if (Piece == Piece::Bishop) Bishop(board, sq, pin, check);
                if (Piece == Piece::Rook  ) Rook  (board, sq, pin, check);
                if (Piece == Piece::Queen ) Queen (board, sq, pin, check);
                if (Piece == Piece::King  ) King  (board, sq            );
            }

            [[nodiscard]]
            constexpr inline uint8_t Count() const
            {
                return ::Count(InternalContainer);
            }

            [[nodiscard]]
            constexpr inline BitBoardIterator Iterator() const
            {
                return BitBoardIterator(InternalContainer);
            }

            constexpr inline MoveList<Piece, Color> Mask(const BitBoard mask) const
            {
                MoveList<Piece, Color> result = *this;
                result.InternalContainer &= mask;
                return result;
            }

    private:
            constexpr inline void Pawn  (const Board&       board, const Square         sq   ,
                                         const PinBitBoard& pin  , const CheckBitBoard& check)
            {
                if        (Get(pin.Diagonal, sq)) {
                    const BitBoard pawnAttack = AttackTable::Pawn[Color][sq];
                    const BitBoard enPassant  = board.EnPassant() & pawnAttack;
                    const BitBoard normal     = pawnAttack & board[Opposite(Color)];

                    InternalContainer |= (enPassant & pin.Diagonal) | (normal & pin.Diagonal & check.Check);

                    if (enPassant) {
                        const Square epTarget  = board.EnPassantSquare();
                        const auto   epPieceSq = static_cast<Square>(
                                Color == White ?
                                epTarget - 8 :
                                epTarget + 8
                        );
                        if (!EnPassantLegal(board, sq, epPieceSq, epTarget))
                            InternalContainer &= ~enPassant;
                    }

                    return;
                } else if (Get(pin.Straight, sq)) {
                    const BitBoard sqBoard = FromSquare(sq);

                    BitBoard pushes = (Color == White ? sqBoard << 8  : sqBoard >> 8 ) & board[NAC];
                    if (pushes &&
                       (sqBoard &     (Color == White ?
                                   BlackMagicFactory::Vertical[1] :
                                   BlackMagicFactory::Vertical[6])))
                            pushes |= (Color == White ? sqBoard << 16 : sqBoard >> 16) & board[NAC];

                    InternalContainer |= pushes & pin.Straight & check.Check;

                    return;
                }

                const BitBoard pawnAttack = AttackTable::Pawn[Color][sq];
                const BitBoard normal     = pawnAttack & board[Opposite(Color)];

                InternalContainer |= normal;

                const BitBoard sqBoard = FromSquare(sq);

                BitBoard pushes = (Color == White ? sqBoard << 8  : sqBoard >> 8 ) & board[NAC];
                if (pushes &&
                   (sqBoard &     (Color == White ?
                               BlackMagicFactory::Vertical[1] :
                               BlackMagicFactory::Vertical[6])))
                        pushes |= (Color == White ? sqBoard << 16 : sqBoard >> 16) & board[NAC];

                InternalContainer |= pushes;

                InternalContainer &= check.Check;

                const BitBoard enPassant = board.EnPassant() & pawnAttack;

                InternalContainer |= enPassant;

                if (enPassant) {
                    const Square epTarget  = board.EnPassantSquare();
                    const auto   epPieceSq = static_cast<Square>(
                            Color == White ?
                            epTarget - 8 :
                            epTarget + 8
                    );
                    if (!EnPassantLegal(board, sq, epPieceSq, epTarget))
                        InternalContainer &= ~enPassant;
                }
            }

            constexpr inline void Knight(const Board&       board, const Square         sq   ,
                                         const PinBitBoard& pin  , const CheckBitBoard& check)
            {
                if (Get(pin.Straight | pin.Diagonal, sq)) return;

                InternalContainer |= AttackTable::Knight[sq] & ~board[Color] & check.Check;
            }

            constexpr inline void Bishop(const Board&       board, const Square         sq   ,
                                         const PinBitBoard& pin  , const CheckBitBoard& check)
            {
                if (Get(pin.Straight, sq)) return;

                const uint32_t idx = BlackMagicFactory::MagicIndex(Piece, sq, ~board[NAC]);
                InternalContainer |= AttackTable::Sliding[idx] & ~board[Color] & check.Check;

                if (Get(pin.Diagonal, sq)) InternalContainer &= pin.Diagonal;
            }

            constexpr inline void Rook  (const Board&       board, const Square         sq   ,
                                         const PinBitBoard& pin  , const CheckBitBoard& check)
            {
                if (Get(pin.Diagonal, sq)) return;

                const uint32_t idx = BlackMagicFactory::MagicIndex(Piece, sq, ~board[NAC]);
                InternalContainer |= AttackTable::Sliding[idx] & ~board[Color] & check.Check;

                if (Get(pin.Straight, sq)) InternalContainer &= pin.Straight;
            }

            constexpr inline void Queen (const Board&       board, const Square         sq   ,
                                         const PinBitBoard& pin  , const CheckBitBoard& check)
            {
                const bool straight = Get(pin.Straight, sq);
                const bool diagonal = Get(pin.Diagonal, sq);

                if (straight && diagonal) return;

                if        (straight) {
                    const uint32_t idx =
                            BlackMagicFactory::MagicIndex(Piece::Rook  , sq, ~board[NAC]);
                    InternalContainer |= AttackTable::Sliding[idx ] & ~board[Color] & check.Check & pin.Straight;

                } else if (diagonal) {
                    const uint32_t idx =
                            BlackMagicFactory::MagicIndex(Piece::Bishop, sq, ~board[NAC]);
                    InternalContainer |= AttackTable::Sliding[idx ] & ~board[Color] & check.Check & pin.Diagonal;

                } else {
                    const uint32_t idxR =
                            BlackMagicFactory::MagicIndex(Piece::Rook  , sq, ~board[NAC]);
                    InternalContainer |= AttackTable::Sliding[idxR] & ~board[Color] & check.Check;

                    const uint32_t idxB =
                            BlackMagicFactory::MagicIndex(Piece::Bishop, sq, ~board[NAC]);
                    InternalContainer |= AttackTable::Sliding[idxB] & ~board[Color] & check.Check;
                }
            }

            constexpr inline void King  (const Board&       board, const Square         sq   )
            {
                BitBoard king = AttackTable::King[sq] & ~board[Color];

                if (!king) return;

                auto iterator = BitBoardIterator(king);
                for (Square target = iterator.Value(); target != NASQ; target = iterator.Value()) {
                    if (!KingMoveLegal(board, target)) Set<false>(king, target);
                }

                InternalContainer |= king;

                if (!KingMoveLegal(board, sq)) return;

                const bool kingSide  = board.CastlingRightK<Color>();
                const bool queenSide = board.CastlingRightQ<Color>();

                if (queenSide &&
                    Get(king, static_cast<Square>(sq - 1)) &&
                    KingMoveLegal(board, static_cast<Square>(sq - 2))) {
                    const BitBoard path = Color == White ? WhiteQueenCastlePath : BlackQueenCastlePath;

                    if (!(path & ~board[NAC])) InternalContainer |= path & QueenCastlePathMask;
                }

                if (kingSide &&
                    Get(king, static_cast<Square>(sq + 1)) &&
                    KingMoveLegal(board, static_cast<Square>(sq + 2))) {
                    const BitBoard path = Color == White ? WhiteKingCastlePath : BlackKingCastlePath;

                    if (!(path & ~board[NAC])) InternalContainer |= path;
                }
            }

            constexpr static inline bool KingMoveLegal (const Board& board, const Square target) {
                constexpr enum Color by = Opposite(Color);

                if (AttackTable::Pawn[Color][target] & board.PieceBoard(Piece::Pawn, by))
                    return false;

                if (AttackTable::Knight[target] & board.PieceBoard(Piece::Knight, by))
                    return false;

                const BitBoard occupied = ~board[Color::NAC] & ~board.PieceBoard(Piece::King, Color);

                const BitBoard queen = board.PieceBoard(Piece::Queen, by);

                if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Bishop, target, occupied)] &
                    (queen | board.PieceBoard(Piece::Bishop, by))) return false;

                if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Rook  , target, occupied)] &
                    (queen | board.PieceBoard(Piece::Rook  , by))) return false;

                return !(AttackTable::King[target] & board.PieceBoard(Piece::King, by));
            }

            constexpr static inline bool EnPassantLegal(const Board& board,
                                                        const Square ourPawn,
                                                        const Square opponentPawn,
                                                        const Square enPassantTarget)
            {
                BitBoard occupied = ~board[Color::NAC];
                Set<false>(occupied,         ourPawn);
                Set<false>(occupied,    opponentPawn);
                Set<true >(occupied, enPassantTarget);

                const BitBoard queen =              board.PieceBoard(Piece::Queen, Opposite(Color));
                const Square   king  = ToSquare(board.PieceBoard(Piece::King ,             Color));

                return !((AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Rook  , king, occupied)] &
                         (queen | board.PieceBoard(Piece::Rook  , Opposite(Color)))) ||
                         (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Bishop, king, occupied)] &
                         (queen | board.PieceBoard(Piece::Bishop, Opposite(Color)))));
            }

    };

} // StockDory

#endif //STOCKDORY_MOVELIST_H
