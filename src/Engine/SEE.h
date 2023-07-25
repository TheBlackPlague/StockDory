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
    };

} // StockDory

#endif //STOCKDORY_SEE_H
