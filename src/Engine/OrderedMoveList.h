//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_ORDEREDMOVELIST_H
#define STOCKDORY_ORDEREDMOVELIST_H

#include <array>
#include <cassert>

#include "../Backend/Move/MoveList.h"
#include "../Backend/Type/Move.h"

#include "Common.h"

#include "Policy.h"

namespace StockDory
{

    template<Color Color, bool CaptureOnly = false, bool UsePolicy = true>
    class OrderedMoveList
    {

        struct OrderedMove
        {

            int32_t Score;
            Move    Move ;

            OrderedMove() = default;

            OrderedMove(const int32_t score, const ::Move move) : Score(score), Move(move) {}

            OrderedMove(const OrderedMove& other) : Score(other.Score), Move(other.Move) {}

        };

        using OrderingPolicy = Policy<Color, CaptureOnly>;

        Array<OrderedMove, MaxMove> Internal;
        uint8_t                     Size = 0;

        public:
        explicit OrderedMoveList(
            const Board & board , const uint8_t ply   ,
            const KTable& kTable, const HTable& hTable,
            const HTable& cTable, const Move    ttMove = {}
        ) requires UsePolicy
        {
            const Move kOne = kTable[0][ply];
            const Move kTwo = kTable[1][ply];

            const Policy<Color, CaptureOnly> policy (board, kOne, kTwo, hTable, cTable, ttMove);

            const PinBitBoard pin = board.Pin<Color, Opposite(Color)>();

            if (const CheckBitBoard check = board.Check<Opposite(Color)>(); check.DoubleCheck) {
                AddMoveLoop<King  >(board, policy, pin, check);
            } else {
                AddMoveLoop<Pawn  >(board, policy, pin, check);
                AddMoveLoop<Knight>(board, policy, pin, check);
                AddMoveLoop<Bishop>(board, policy, pin, check);
                AddMoveLoop<Rook  >(board, policy, pin, check);
                AddMoveLoop<Queen >(board, policy, pin, check);
                AddMoveLoop<King  >(board, policy, pin, check);
            }
        }

        explicit OrderedMoveList(const Board& board) requires (!UsePolicy)
        {
            const PinBitBoard pin = board.Pin<Color, Opposite(Color)>();

            if (const CheckBitBoard check = board.Check<Opposite(Color)>(); check.DoubleCheck) {
                AddMoveLoop<King  >(board, pin, check);
            } else {
                AddMoveLoop<Pawn  >(board, pin, check);
                AddMoveLoop<Knight>(board, pin, check);
                AddMoveLoop<Bishop>(board, pin, check);
                AddMoveLoop<Rook  >(board, pin, check);
                AddMoveLoop<Queen >(board, pin, check);
                AddMoveLoop<King  >(board, pin, check);
            }
        }

        private:
        template<Piece Piece>
        void AddMoveLoop(
            const Board         &  board,
            const OrderingPolicy& policy,
            const PinBitBoard   &    pin,
            const CheckBitBoard &  check
        ) requires UsePolicy
        {
            BitBoardIterator iterator (board.PieceBoard<Color>(Piece));

            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const MoveList<Piece, Color> moves (board, sq, pin, check);
                BitBoardIterator             moveIterator =
                    CaptureOnly ? (Piece == Pawn
                    ? moves.Mask(~board[NAC] | board.EnPassant())
                    : moves.Mask(~board[NAC])).Iterator()
                    : moves.Iterator();

                for (Square m = moveIterator.Value(); m != NASQ; m = moveIterator.Value()) {
                    if (moves.Promotion(sq)) {
                        Internal[Size++] = CreateOrdered<Piece, Queen >(board, policy, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Knight>(board, policy, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Rook  >(board, policy, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Bishop>(board, policy, sq, m);
                    } else
                        Internal[Size++] = CreateOrdered<Piece        >(board, policy, sq, m);
                }
            }
        }

        template<Piece Piece>
        void AddMoveLoop(const Board&  board, const PinBitBoard& pin, const CheckBitBoard &check) requires (!UsePolicy)
        {
            BitBoardIterator iterator (board.PieceBoard<Color>(Piece));

            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const MoveList<Piece, Color> moves (board, sq, pin, check);
                BitBoardIterator             moveIterator =
                    CaptureOnly ? (Piece == Pawn
                    ? moves.Mask(~board[NAC] | board.EnPassant())
                    : moves.Mask(~board[NAC])).Iterator()
                    : moves.Iterator();

                for (Square m = moveIterator.Value(); m != NASQ; m = moveIterator.Value()) {
                    if (moves.Promotion(sq)) {
                        Internal[Size++] = CreateOrdered<Piece, Queen >(board, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Knight>(board, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Rook  >(board, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Bishop>(board, sq, m);
                    } else
                        Internal[Size++] = CreateOrdered<Piece        >(board, sq, m);
                }
            }
        }

        // ReSharper disable once CppRedundantElaboratedTypeSpecifier
        template<Piece Piece, enum Piece Promotion = NAP>
        static OrderedMove CreateOrdered(
            const Board         &  board,
            const OrderingPolicy& policy,
            const Square           from ,
            const Square            to
        )
        {
            const auto move = board.CreateMove<Piece>(from, to, Promotion);
            return { policy.template Score<Piece, Promotion>(move), move };
        }

        template<Piece Piece, enum Piece Promotion = NAP>
        static OrderedMove CreateOrdered(const Board& board, const Square from, const Square to) requires (!UsePolicy)
        {
            const auto move = board.CreateMove<Piece>(from, to, Promotion);
            return { 0, move };
        }

        public:
        [[nodiscard]]
        Move operator [](const uint8_t index)
        {
            assert(index < Size);

            if constexpr (UsePolicy) SortNext(index);

            return Internal[index].Move;
        }

        [[nodiscard]]
        Move UnsortedAccess(const uint8_t index) const
        {
            assert(index < Size);

            return Internal[index].Move;
        }

        [[nodiscard]]
        uint8_t Count() const
        {
            return Size;
        }

        private:
        void SortNext(const uint8_t sorted) requires UsePolicy
        {
            int32_t best = Internal[sorted].Score << 8 | (MaxMove - sorted);
            for (uint8_t i = sorted + 1; i < Size; i++) best = std::max(best, Internal[i].Score << 8 | (MaxMove - i));

            const uint8_t index = MaxMove - (best & 0xFF);

            std::swap(Internal[index], Internal[sorted]);
        }

    };

} // StockDory

#endif //STOCKDORY_ORDEREDMOVELIST_H
