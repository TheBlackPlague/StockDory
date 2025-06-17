//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVELIST_H
#define STOCKDORY_MOVELIST_H

#include "Move.h"
#include "Position.h"

namespace StockDory
{

    template<Side Side, PieceType Type>
    class PieceMoveList
    {

        BitBoard Internal;

        public:
        PieceMoveList() = delete;

        template<class EvaluationHandler = DefaultEvaluationHandler>
        PieceMoveList(const Position<EvaluationHandler>& position, const Square sq)
        : PieceMoveList(position, position.template PinBB<Side>(), position.template CheckBB<Side>(), sq) {}

        template<class EvaluationHandler = DefaultEvaluationHandler>
        PieceMoveList(const Position<EvaluationHandler>& position, const PinBitBoard& pin, const CheckBitBoard& check,
                      const Square sq)
        {

        }

        private:
        template<class EvaluationHandler = DefaultEvaluationHandler>
        [[clang::always_inline]]
        static bool EnPassantLegal(const Position<EvaluationHandler>& position,
                                   const Square x, const Square y, const Square z)
        {
            // TODO: Replace usage of InvalidSide BB
            BitBoard occ = ~position[InvalidSide];

            Set<false>(occ, x);
            Set<false>(occ, y);
            Set<true >(occ, z);

            const Square king = AsSquare(position[Side, King]);

            return !(
                Attack::Sliding<Bishop>(king, occ) & (position[~Side, Queen] | position[~Side, Bishop]) ||
                Attack::Sliding< Rook >(king, occ) & (position[~Side, Queen] | position[~Side,  Rook ])
            );
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        static bool KingSquareLegal(const Position<EvaluationHandler>& position, const Square sq)
        {
            if (Attack::Pawn[Side][sq] & position[~Side, Pawn]) return false;

            if (Attack::Knight[sq] & position[~Side, Knight]) return false;

            // TODO: Replace usage of InvalidSide BB
            const BitBoard occ = ~position[InvalidSide] & ~position[Side, King];

            if (Attack::Sliding<Bishop>(sq, occ) & (position[~Side, Queen] | position[~Side, Bishop])) return false;
            if (Attack::Sliding< Rook >(sq, occ) & (position[~Side, Queen] | position[~Side,  Rook ])) return false;

            return !(Attack::King[sq] & position[~Side, King]);
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        [[clang::always_inline]]
        void PawnMove(const Position<EvaluationHandler>& position, const PinBitBoard& pin, const CheckBitBoard& check,
                      const Square sq)
        {
            const auto property = position.PositionProperty();

            BitBoard pushes {};

            if (!Get(pin.DiagonalMask, sq)) {
                // TODO: Replace usage of InvalidSide BB
                pushes |= (Side == White ? AsBitBoard(sq) << 8 : AsBitBoard(sq) >> 8) & position[InvalidSide];
                if (pushes && pushes & (Side == White ? Ray::Rank[Ray::Rank3] : Ray::Rank[Ray::Rank6]))
                    pushes |= (Side == White ? pushes << 8 : pushes >> 8) & position[InvalidSide];
            }

            Internal |= pushes & pin.StraightMask;

            if (Get(pin.StraightMask, sq)) { Internal &= check.Mask; return; }

            const BitBoard attack = Attack::Pawn[Side][sq];

            const Square ep = property.EnPassantSquare();

            Internal |= attack & AsBitBoard(ep) & pin.DiagonalMask;

            if (Internal) {
                const auto epPieceSq = static_cast<Square>(
                    Side == White ? ep - 8 : ep + 8
                );

                if (!EnPassantLegal(position, sq, epPieceSq, ep)) Internal &= ~AsBitBoard(ep);
            }

            Internal |= attack & position[~Side] & pin.DiagonalMask & check.Mask;
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        void KnightMove(const Position<EvaluationHandler>& position, const PinBitBoard& pin, const CheckBitBoard& check,
                        const Square sq)
        {
            if (Get(pin.StraightMask | pin.DiagonalMask, sq)) return;

            Internal |= Attack::Knight[sq] & ~position[Side] & check.Mask;
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        void BishopMove(const Position<EvaluationHandler>& position, const PinBitBoard& pin, const CheckBitBoard& check,
                        const Square sq)
        {
            if (Get(pin.StraightMask, sq)) return;

            // TODO: Replace usage of InvalidSide BB
            Internal = Attack::Sliding<Bishop>(sq, ~position[InvalidSide]) & ~position[Side] & check.Mask;

            if (Get(pin.DiagonalMask, sq)) Internal &= pin.DiagonalMask;
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        void RookMove(const Position<EvaluationHandler>& position, const PinBitBoard& pin, const CheckBitBoard& check,
                      const Square sq)
        {
            if (Get(pin.DiagonalMask, sq)) return;

            // TODO: Replace usage of InvalidSide BB
            Internal = Attack::Sliding<Rook>(sq, ~position[InvalidSide]) & ~position[Side] & check.Mask;

            if (Get(pin.StraightMask, sq)) Internal &= pin.StraightMask;
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        void QueenMove(const Position<EvaluationHandler>& position, const PinBitBoard& pin, const CheckBitBoard& check,
                       const Square sq)
        {
            const bool straight = Get(pin.StraightMask, sq),
                       diagonal = Get(pin.DiagonalMask, sq);

            if (straight && diagonal) return;

            if      (diagonal)
                // TODO: Replace usage of InvalidSide BB
                Internal = Attack::Sliding<Bishop>(sq, ~position[InvalidSide]) & pin.DiagonalMask;
            else if (straight)
                // TODO: Replace usage of InvalidSide BB
                Internal = Attack::Sliding< Rook >(sq, ~position[InvalidSide]) & pin.StraightMask;
            else {
                // TODO: Replace usage of InvalidSide BB
                Internal = Attack::Sliding<Bishop>(sq, ~position[InvalidSide]) &
                           Attack::Sliding< Rook >(sq, ~position[InvalidSide]) ;
            }

            Internal &= ~position[Side] & check.Mask;
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        void KingMove(const Position<EvaluationHandler>& position, const CheckBitBoard& check, const Square sq)
        {
            BitBoard attack = Attack::King[sq] & ~position[Side];

            if (!attack) return;

            for (const auto target : attack) if (!KingSquareLegal(position, target)) Set<false>(attack, target);

            Internal |= attack;

            if (check.Mask != ~0ULL) return;

            const auto property = position.PositionProperty();

            const bool  kingSide = property.template CanCastle<Side, K>(),
                       queenSide = property.template CanCastle<Side, Q>();

            if (kingSide && Get(Internal, static_cast<Square>(sq + 1)) &&
                KingSquareLegal(position, static_cast<Square>(sq + 2))) {
                constexpr BitBoard path = 0b01100000 << (Side == White ? 0 : 56);

                // TODO: Replace usage of InvalidSide BB
                if (!(path & ~position[InvalidSide])) Internal |= path;
            }

            if (queenSide && Get(Internal, static_cast<Square>(sq - 1)) &&
                 KingSquareLegal(position, static_cast<Square>(sq - 2))) {
                constexpr BitBoard path = 0b00001110 << (Side == White ? 0 : 56);

                // TODO: Replace usage of InvalidSide BB
                if (!(path & ~position[InvalidSide])) Internal |= path & 0b00001100 << (Side == White ? 0 : 56);
            }
        }

    };

    constexpr u08 MaxMove = 218; // Maximum number of legal moves in a position

    template<Side Side>
    class MoveList
    {

        Array<Move<>, MaxMove> Moves;

        u08 Size = 0;

        public:
        MoveList() = delete;

        template<class EvaluationHandler = DefaultEvaluationHandler>
        MoveList(const Position<EvaluationHandler>& position)
        {
            const auto   pin = position.template   PinBB<Side>();
            const auto check = position.template CheckBB<Side>();

            if (check.Double) {

            } else {

            }
        }

        template<class EvaluationHandler = DefaultEvaluationHandler>
        void Pawn()
        {

        }

        constexpr MoveList begin() const { return Moves.begin()       ; }
        constexpr MoveList   end() const { return Moves.begin() + Size; }

    };

} // StockDory

#endif //STOCKDORY_MOVELIST_H
