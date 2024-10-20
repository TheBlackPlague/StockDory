//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SIMPLIFIEDMOVELIST_H
#define STOCKDORY_SIMPLIFIEDMOVELIST_H

#include <array>
#include <cassert>

#include "Backend/Move/MoveList.h"
#include "Backend/Type/Move.h"

namespace StockDory
{

    template<Color Color, bool CaptureOnly = false>
    class SimplifiedMoveList
    {

    private:
        static constexpr int MaxMove = 256;
        std::array<Move, MaxMove> Internal = {};  // Array of moves without ordering
        uint8_t Size = 0;

    public:
        explicit SimplifiedMoveList(const Board& board)
        {
            const PinBitBoard   pin   = board.Pin  <Color, Opposite(Color)>();
            const CheckBitBoard check = board.Check<Opposite(Color)>();

            if (check.DoubleCheck) {
                AddMoveLoop<King>(board, pin, check);
            } else {
                AddMoveLoop<Pawn>(board, pin, check);
                AddMoveLoop<Knight>(board, pin, check);
                AddMoveLoop<Bishop>(board, pin, check);
                AddMoveLoop<Rook>(board, pin, check);
                AddMoveLoop<Queen>(board, pin, check);
                AddMoveLoop<King>(board, pin, check);
            }
        }

        template<Piece Piece>
        inline void AddMoveLoop(const Board& board,
                                const PinBitBoard& pin,
                                const CheckBitBoard& check)
        {
            BitBoardIterator iterator(board.PieceBoard<Color>(Piece));

            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const MoveList<Piece, Color> moves(board, sq, pin, check);
                BitBoardIterator moveIterator = CaptureOnly ?
                        (Piece == Pawn ?
                         moves.Mask(~board[NAC] | board.EnPassant()) :
                         moves.Mask(~board[NAC])).Iterator() :
                         moves.Iterator();

                for (Square m = moveIterator.Value(); m != NASQ; m = moveIterator.Value()) {
                    if (moves.Promotion(sq)) {
                        Internal[Size++] = CreateMove<Piece, Queen>(sq, m);
                        Internal[Size++] = CreateMove<Piece, Knight>(sq, m);
                        Internal[Size++] = CreateMove<Piece, Rook>(sq, m);
                        Internal[Size++] = CreateMove<Piece, Bishop>(sq, m);
                    } else {
                        Internal[Size++] = CreateMove<Piece>(sq, m);
                    }
                }
            }
        }

    private:
        template<Piece Piece, enum Piece Promotion = NAP>
        inline Move CreateMove(const Square from, const Square to)
        {
            return Move(from, to, Promotion);  // Simplified move creation without ordering
        }

    public:
        [[nodiscard]]
        inline Move operator [](const uint8_t index) const
        {
            assert(index < Size);
            return Internal[index];
        }

        [[nodiscard]]
        inline uint8_t Count() const
        {
            return Size;
        }

    };

} // StockDory

#endif //STOCKDORY_SIMPLIFIEDMOVELIST_H
