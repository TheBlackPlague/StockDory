//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
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

    template<Color Color, bool NNPolicy, bool CaptureOnly = false>
    class OrderedMoveList
    {

        struct OrderedMove
        {

            uint32_t Score;
            Move    Move ;

            OrderedMove() = default;

            OrderedMove(const uint32_t score, const ::Move move) : Score(score), Move(move) {}

            OrderedMove(const OrderedMove& other) : Score(other.Score), Move(other.Move) {}

        };

        using OrderingPolicy = Policy<Color, NNPolicy, CaptureOnly>;

        Array<OrderedMove, MaxMove> Internal;
        uint8_t                     Size = 0;

        public:
        explicit OrderedMoveList(      Board & board       , const uint8_t ply                 ,
                                 const KTable& kTable      , const HTable& hTable              ,
                                 const Move    ttMove = {} , const Score   staticEvaluation = 0,
                                 const size_t  threadId = 0)
        {
            const Move kOne = kTable[0][ply];
            const Move kTwo = kTable[1][ply];

            Score scaledStaticEvaluation = WDLCalculator::S(board, staticEvaluation);

            const Policy<Color, NNPolicy, CaptureOnly> policy (kOne, kTwo, ttMove, scaledStaticEvaluation, threadId);

            const PinBitBoard   pin   = board.Pin<Color, Opposite(Color)>();

            if (const CheckBitBoard check = board.Check<Opposite(Color)>(); check.DoubleCheck) {
                AddMoveLoop<King  >(board, hTable, policy, pin, check);
            } else {
                AddMoveLoop<Pawn  >(board, hTable, policy, pin, check);
                AddMoveLoop<Knight>(board, hTable, policy, pin, check);
                AddMoveLoop<Bishop>(board, hTable, policy, pin, check);
                AddMoveLoop<Rook  >(board, hTable, policy, pin, check);
                AddMoveLoop<Queen >(board, hTable, policy, pin, check);
                AddMoveLoop<King  >(board, hTable, policy, pin, check);
            }
        }

        template<Piece Piece>
        void AddMoveLoop(      Board         &  board,
                         const HTable        & hTable,
                         const OrderingPolicy& policy,
                         const PinBitBoard   &    pin,
                         const CheckBitBoard &  check)
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
                        Internal[Size++] = CreateOrdered<Piece, Queen >(board, hTable, policy, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Knight>(board, hTable, policy, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Rook  >(board, hTable, policy, sq, m);
                        Internal[Size++] = CreateOrdered<Piece, Bishop>(board, hTable, policy, sq, m);
                    } else
                        Internal[Size++] = CreateOrdered<Piece        >(board, hTable, policy, sq, m);
                }
            }
        }

        private:
        // ReSharper disable once CppRedundantElaboratedTypeSpecifier
        template<Piece Piece, enum Piece Promotion = NAP>
        static OrderedMove CreateOrdered(      Board         &  board,
                                         const HTable        & hTable,
                                         const OrderingPolicy& policy,
                                         const Square           from ,
                                         const Square            to  )
        {
            const auto move = Move(from, to, Promotion);
            return { policy.template Score<Piece, Promotion>(board, hTable, move), move };
        }

        public:
        [[nodiscard]]
        Move operator [](const uint8_t index)
        {
            assert(index < Size);

            SortNext(index);
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
        void SortNext(const uint8_t sorted)
        {
            uint8_t index = sorted;
            uint8_t i     = sorted + 1;

            while (i < Size) {
                if (Internal[i].Score > Internal[index].Score) index = i;
                i++;
            }

            const OrderedMove temp = Internal[ index];
            Internal[ index]       = Internal[sorted];
            Internal[sorted]       = temp;
        }

    };

} // StockDory

#endif //STOCKDORY_ORDEREDMOVELIST_H
