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

#include "../Board.h"

#include "AttackTable.h"

namespace StockDory
{

    template<Color C>
    class MoveList
    {

        private:
            std::array<Move, 128> Moves;

        public:
            constexpr MoveList(const Board& board)
            {

            }

            constexpr void Pawn()
            {

            }

            constexpr void Knight()
            {

            }

            constexpr void Bishop()
            {

            }

            constexpr void Rook()
            {

            }

            constexpr void Queen()
            {

            }

            constexpr void King()
            {

            }

    };

} // StockDory

#endif //STOCKDORY_MOVELIST_H
