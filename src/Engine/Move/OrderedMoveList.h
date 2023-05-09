//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_ORDEREDMOVELIST_H
#define STOCKDORY_ORDEREDMOVELIST_H

#include <array>
#include <cassert>

#include "../EngineParameter.h"
#include "Policy.h"
#include "KillerTable.h"
#include "HistoryTable.h"

#include "../../Backend/Move/MoveList.h"
#include "../../Backend/Type/Move.h"

namespace StockDory
{

    template<Color Color, bool CaptureOnly = false>
    class OrderedMoveList
    {

        using OrderedMove = std::pair<int32_t, Move>;

        private:
            std::array<OrderedMove, MaxMove> Internal = {};
            uint8_t Size = 0;

        public:
            explicit OrderedMoveList(const Board& board, const uint8_t ply,
                                     const KillerTable& kTable, const HistoryTable& hTable,
                                     const Move ttMove)
            {
                const Move kOne = kTable.Get<1>(ply);
                const Move kTwo = kTable.Get<2>(ply);

                const Policy<Color, CaptureOnly> policy (kOne, kTwo, ttMove);

                const PinBitBoard   pin   = board.Pin  <Color, Opposite(Color)>();
                const CheckBitBoard check = board.Check<       Opposite(Color)>();

                if (check.DoubleCheck) {
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
            inline void AddMoveLoop(const Board& board, const HistoryTable& hTable,
                                    const Policy<Color, CaptureOnly>& policy,
                                    const PinBitBoard& pin, const CheckBitBoard& check)
            {
                BitBoardIterator iterator (board.PieceBoard<Color>(Piece));

                for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                    const MoveList<Piece, Color> moves (board, sq, pin, check);
                    BitBoardIterator moveIterator = CaptureOnly ?
                            (Piece == Pawn ?
                             moves.Mask(~board[NAC] | board.EnPassant()) :
                             moves.Mask(~board[NAC])).Iterator() :
                             moves                         .Iterator() ;

                    for (Square m = moveIterator.Value(); m != NASQ; m = moveIterator.Value()) {
                        if (moves.Promotion(sq)) {
                            Internal[Size++] = CreateOrdered<Piece, Queen >(board, hTable, policy, sq, m);
                            Internal[Size++] = CreateOrdered<Piece, Rook  >(board, hTable, policy, sq, m);
                            Internal[Size++] = CreateOrdered<Piece, Bishop>(board, hTable, policy, sq, m);
                            Internal[Size++] = CreateOrdered<Piece, Knight>(board, hTable, policy, sq, m);
                        } else {
                            Internal[Size++] = CreateOrdered<Piece        >(board, hTable, policy, sq, m);
                        }
                    }
                }
            }

        private:
            template<Piece Piece, enum Piece Promotion = NAP>
            inline std::pair<int32_t, Move> CreateOrdered(const Board& board,
                                                          const HistoryTable& hTable,
                                                          const Policy<Color, CaptureOnly>& policy,
                                                          const Square from, const Square to)
            {
                const Move move = Move(from, to, Promotion);
                return {policy.template Score<Piece, Promotion>(board, hTable, move), move };
            }

        public:
            [[nodiscard]]
            inline Move operator [](const uint8_t index)
            {
                assert(index < Size);

                SortNext(index);
                return Internal[index].second;
            }

            [[nodiscard]]
            inline Move UnsortedAccess(const uint8_t index) const
            {
                assert(index < Size);

                return Internal[index].second;
            }

            [[nodiscard]]
            inline uint8_t Count() const
            {
                return Size;
            }

        private:
            inline void SortNext(const uint8_t sorted)
            {
                uint8_t index = sorted    ;
                uint8_t i     = sorted + 1;

                while (i < Size) {
                    if (Internal[i].first > Internal[index].first) index = i;
                    i++;
                }

                const OrderedMove temp = Internal[index];
                Internal[index ] = Internal[sorted];
                Internal[sorted] = temp;
            }

    };

} // StockDory

#endif //STOCKDORY_ORDEREDMOVELIST_H
