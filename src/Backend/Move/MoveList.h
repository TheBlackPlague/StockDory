//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVELIST_H
#define STOCKDORY_MOVELIST_H

#include <array>

#include "../Type/BitBoard.h"
#include "../Type/CheckBitBoard.h"
#include "../Type/Color.h"
#include "../Type/Piece.h"
#include "../Type/PinBitBoard.h"
#include "../Type/Square.h"

#include "../Board.h"

#include "AttackTable.h"
#include "BlackMagicFactory.h"

namespace StockDory
{

    template<Piece Piece, Color Color>
    class MoveList
    {

        constexpr static BitBoard WhiteQueenCastlePath = 0x0EULL;
        constexpr static BitBoard WhiteKingCastlePath  = 0x60ULL;
        constexpr static BitBoard BlackQueenCastlePath = WhiteQueenCastlePath << 56;
        constexpr static BitBoard BlackKingCastlePath  = WhiteKingCastlePath  << 56;

        constexpr static BitBoard QueenCastlePathMask = 0x0C0000000000000CULL;

        BitBoard InternalContainer;

        public:
        constexpr static bool Promotion(const Square sq)
        {
            if (Piece != Piece::Pawn) return false;

            return (Color == White && sq > H6) || (Color == Black && sq < A3);
        }

        MoveList(const Board& board, const Square sq, const PinBitBoard& pin, const CheckBitBoard& check)
        {
            InternalContainer = BBDefault;

            if (Piece == ::Pawn  ) Pawn   (board, pin, check, sq);
            if (Piece == ::Knight) Knight (board, pin, check, sq);
            if (Piece == ::Bishop) Bishop (board, pin, check, sq);
            if (Piece == ::Rook  ) Rook   (board, pin, check, sq);
            if (Piece == ::Queen ) Queen  (board, pin, check, sq);
            if (Piece == ::King  ) King   (board,             sq);
        }

        [[nodiscard]]
        uint8_t Count() const
        {
            return ::Count(InternalContainer);
        }

        [[nodiscard]]
        BitBoardIterator Iterator() const
        {
            return BitBoardIterator(InternalContainer);
        }

        MoveList Mask(const BitBoard mask) const
        {
            MoveList result = *this;
            result.InternalContainer &= mask;
            return result;
        }

        private:
        [[clang::always_inline]]
        void Pawn  (const Board& board, const PinBitBoard& pin, const CheckBitBoard& check, const Square sq)
        {
            if (Get(pin.Diagonal, sq)) {
                const BitBoard pawnAttack = AttackTable::Pawn[Color][sq];
                const BitBoard enPassant  = board.EnPassant() & pawnAttack;
                const BitBoard normal     = pawnAttack & board[Opposite(Color)];

                InternalContainer |= enPassant & pin.Diagonal | normal & pin.Diagonal & check.Check;

                if (enPassant) {
                    const Square epTarget  = board.EnPassantSquare();
                    const auto   epPieceSq = static_cast<Square>(
                        Color == White ? epTarget - 8 : epTarget + 8
                    );
                    if (!EnPassantLegal(board, sq, epPieceSq, epTarget))
                        InternalContainer &= ~enPassant;
                }

                return;
            }

            if (Get(pin.Straight, sq)) {
                const BitBoard sqBoard = FromSquare(sq);

                BitBoard pushes = (Color == White ? sqBoard << 8 : sqBoard >> 8) & board[NAC];
                if (pushes &&
                    sqBoard & (Color == White ? RayTable::Vertical[1] : RayTable::Vertical[6]))
                    pushes |= (Color == White ? sqBoard << 16 : sqBoard >> 16) & board[NAC];

                InternalContainer |= pushes & pin.Straight & check.Check;

                return;
            }

            const BitBoard pawnAttack = AttackTable::Pawn[Color][sq];
            const BitBoard normal     = pawnAttack & board[Opposite(Color)];

            InternalContainer |= normal;

            const BitBoard sqBoard = FromSquare(sq);

            BitBoard pushes = (Color == White ? sqBoard << 8 : sqBoard >> 8) & board[NAC];
            if (pushes &&
                sqBoard & (Color == White ? RayTable::Vertical[1] : RayTable::Vertical[6]))
                pushes |= (Color == White ? sqBoard << 16 : sqBoard >> 16) & board[NAC];

            InternalContainer |= pushes;

            InternalContainer &= check.Check;

            const BitBoard enPassant = board.EnPassant() & pawnAttack;

            InternalContainer |= enPassant;

            if (enPassant) {
                const Square epTarget  = board.EnPassantSquare();
                const auto   epPieceSq = static_cast<Square>(
                    Color == White ? epTarget - 8 : epTarget + 8
                );
                if (!EnPassantLegal(board, sq, epPieceSq, epTarget))
                    InternalContainer &= ~enPassant;
            }
        }

        [[clang::always_inline]]
        void Knight(const Board& board, const PinBitBoard& pin, const CheckBitBoard& check, const Square sq)
        {
            if (Get(pin.Straight | pin.Diagonal, sq)) return;

            InternalContainer |= AttackTable::Knight[sq] & ~board[Color] & check.Check;
        }

        [[clang::always_inline]]
        void Bishop(const Board& board, const PinBitBoard& pin, const CheckBitBoard& check, const Square sq)
        {
            if (Get(pin.Straight, sq)) return;

            const uint32_t idx = BlackMagicFactory::MagicIndex(Piece, sq, ~board[NAC]);
            InternalContainer |= AttackTable::Sliding[idx] & ~board[Color] & check.Check;

            if (Get(pin.Diagonal, sq)) InternalContainer &= pin.Diagonal;
        }

        [[clang::always_inline]]
        void Rook  (const Board& board, const PinBitBoard& pin, const CheckBitBoard& check, const Square sq)
        {
            if (Get(pin.Diagonal, sq)) return;

            const uint32_t idx = BlackMagicFactory::MagicIndex(Piece, sq, ~board[NAC]);
            InternalContainer |= AttackTable::Sliding[idx] & ~board[Color] & check.Check;

            if (Get(pin.Straight, sq)) InternalContainer &= pin.Straight;
        }

        [[clang::always_inline]]
        void Queen (const Board& board, const PinBitBoard& pin, const CheckBitBoard& check, const Square sq)
        {
            const bool straight = Get(pin.Straight, sq);
            const bool diagonal = Get(pin.Diagonal, sq);

            if (straight && diagonal) return;

            if (straight) {
                const uint32_t idx = BlackMagicFactory::MagicIndex(Piece::Rook  , sq, ~board[NAC]);
                InternalContainer |= AttackTable::Sliding[idx] & ~board[Color] & check.Check & pin.Straight;
            } else if (diagonal) {
                const uint32_t idx = BlackMagicFactory::MagicIndex(Piece::Bishop, sq, ~board[NAC]);
                InternalContainer |= AttackTable::Sliding[idx] & ~board[Color] & check.Check & pin.Diagonal;
            } else {
                const uint32_t idxR = BlackMagicFactory::MagicIndex(Piece::Rook  , sq, ~board[NAC]);
                const uint32_t idxB = BlackMagicFactory::MagicIndex(Piece::Bishop, sq, ~board[NAC]);

                InternalContainer |= AttackTable::Sliding[idxR] & ~board[Color] & check.Check;
                InternalContainer |= AttackTable::Sliding[idxB] & ~board[Color] & check.Check;
            }
        }

        [[clang::always_inline]]
        void King  (const Board& board,                                                     const Square sq)
        {
            BitBoard king = AttackTable::King[sq] & ~board[Color];

            if (!king) return;

            auto iterator = BitBoardIterator(king);
            for (Square target = iterator.Value(); target != NASQ; target = iterator.Value()) {
                if (!KingMoveLegal(board, target)) Set<false>(king, target);
            }

            InternalContainer |= king;

            if (!KingMoveLegal(board, sq)) return;

            const bool  kingSide = board.CastlingRightK<Color>(),
                       queenSide = board.CastlingRightQ<Color>();

            if (queenSide &&
                Get(king, static_cast<Square>(sq - 1)) &&
                KingMoveLegal(board, static_cast<Square>(sq - 2)))
                if (const BitBoard path = Color == White ? WhiteQueenCastlePath : BlackQueenCastlePath;
                    !(path & ~board[NAC]))
                    InternalContainer |= path & QueenCastlePathMask;

            if (kingSide  &&
                Get(king, static_cast<Square>(sq + 1)) &&
                KingMoveLegal(board, static_cast<Square>(sq + 2)))
                if (const BitBoard path = Color == White ? WhiteKingCastlePath : BlackKingCastlePath;
                    !(path & ~board[NAC]))
                    InternalContainer |= path;
        }

        [[clang::always_inline]]
        static bool KingMoveLegal(const Board& board, const Square target)
        {
            constexpr auto by = Opposite(Color);

            if (AttackTable::Pawn[Color][target] & board.PieceBoard(Piece::Pawn, by))
                return false;

            if (AttackTable::Knight[target] & board.PieceBoard(Piece::Knight, by))
                return false;

            const BitBoard occupied = ~board[NAC] & ~board.PieceBoard(Piece::King, Color);

            const BitBoard queen = board.PieceBoard(Piece::Queen, by);

            if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Bishop, target, occupied)] &
                (queen | board.PieceBoard(Piece::Bishop, by)))
                return false;

            if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Rook  , target, occupied)] &
                (queen | board.PieceBoard(Piece::Rook  , by)))
                return false;

            return !(AttackTable::King[target] & board.PieceBoard(Piece::King, by));
        }

        [[clang::always_inline]]
        static bool EnPassantLegal(const Board& board,
                                   const Square x, // the square that is moving
                                   const Square y, // the square that is being captured
                                   const Square z) // the square that is being moved to
        {
            BitBoard occupied = ~board[NAC];
            Set<false>(occupied, x);
            Set<false>(occupied, y);
            Set<true >(occupied, z);

            const BitBoard queen = board.PieceBoard(Piece::Queen, Opposite(Color));
            const Square   king  = ToSquare(board.PieceBoard(Piece::King, Color));

            return !(AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Rook  , king, occupied)] &
                     (queen | board.PieceBoard(Piece::Rook, Opposite(Color))) ||
                     AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Bishop, king, occupied)] &
                     (queen | board.PieceBoard(Piece::Bishop, Opposite(Color))));
        }

    };

} // StockDory

#endif //STOCKDORY_MOVELIST_H
