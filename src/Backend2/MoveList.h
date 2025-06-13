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
        [[clang::always_inline]]
        void Pawn(const Position<EvaluationHandler>& position, const PinBitBoard& pin, const CheckBitBoard& check
                  const Square sq)
        {
            const auto property = position.PositionProperty();

            if (Get(pin.DiagonalMask, sq)) {
                const BitBoard attack = Attack::Pawn[Side][sq];

                const Square ep = property.EnPassantSquare();

                Internal |= attack &  AsBitBoard(ep) & pin.DiagonalMask |
                            attack & position[~Side] & pin.DiagonalMask & check.Mask;

                if (AsBitBoard(ep)) {
                    const auto epPieceSq = static_cast<Square>(
                        Side == White ? ep - 8 : ep + 8
                    );

                    if (!EnPassantLegal(position, sq, epPieceSq, ep)) Internal &= ~AsBitBoard(ep);
                }
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
