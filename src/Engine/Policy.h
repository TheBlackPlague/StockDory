//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_POLICY_H
#define STOCKDORY_POLICY_H

#include "../Backend/Board.h"
#include "../Backend/Type/Move.h"

#include "Common.h"
#include "SEE.h"

namespace StockDory
{

    template<Color Color, bool CaptureOnly = false>
    class Policy
    {

        constexpr static std::array<std::array<uint16_t, 7>, 7> MvvLva = {{
            {2005, 2004, 2003, 2002, 2001, 2000, 0000},
            {3005, 3004, 3003, 3002, 3001, 3000, 0000},
            {4005, 4004, 4003, 4002, 4001, 4000, 0000},
            {5005, 5004, 5003, 5002, 5001, 5000, 0000},
            {6005, 6004, 6003, 6002, 6001, 6000, 0000},
            {7005, 7004, 7003, 7002, 7001, 7000, 0000},
            {0000, 0000, 0000, 0000, 0000, 0000, 0000}
        }};

        constexpr static int32_t Priority = 0x7FFFFFFF;

        constexpr static std::array<int32_t, 5> PromotionPriority = {
            0,
            Priority - 8 + 3,
            Priority - 8 + 1,
            Priority - 8 + 2,
            Priority - 8 + 4
        };

        Move KillerOne;
        Move KillerTwo;

        Move TTMove;

        public:
        Policy(const Move kOne, const Move kTwo, const Move tt)
        {
            KillerOne          = kOne;
            KillerTwo          = kTwo;
            TTMove = tt;
        }

        // ReSharper disable once CppRedundantElaboratedTypeSpecifier
        template<Piece Piece, enum Piece Promotion = NAP>
        int32_t Score(const Board& board, const HTable& historyTable, const Move move) const
        {
            if (move == TTMove) return Priority - 1;

            if (Promotion != NAP) return PromotionPriority[Promotion];

            if (CaptureOnly || board[move.To()].Piece() != NAP) return CaptureScore<Piece>(board, move);

            if (move == KillerOne) return 900000;
            if (move == KillerTwo) return 800000;

            return historyTable[Color][Piece][move.To()];
        }

        template<Piece Piece>
        static int32_t CaptureScore(const Board& board, const Move move)
        {
            if (SEE::Accurate(board, move, 0))
                return MvvLva[board[move.To()].Piece()][Piece] * 1000;

                return MvvLva[board[move.To()].Piece()][Piece] * 300 ;
        }

    };

} // StockDory

#endif //STOCKDORY_POLICY_H
