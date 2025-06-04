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

        constexpr static Array<uint16_t, 7, 7> MvvLva = {{
            {2005, 2004, 2003, 2002, 2001, 2000, 0000},
            {3005, 3004, 3003, 3002, 3001, 3000, 0000},
            {4005, 4004, 4003, 4002, 4001, 4000, 0000},
            {5005, 5004, 5003, 5002, 5001, 5000, 0000},
            {6005, 6004, 6003, 6002, 6001, 6000, 0000},
            {7005, 7004, 7003, 7002, 7001, 7000, 0000},
            {0000, 0000, 0000, 0000, 0000, 0000, 0000}
        }};

        constexpr static uint32_t MaximumScore = std::numeric_limits<uint32_t>::max();

        constexpr static uint32_t ScoreAnchor = 1000000;

        constexpr static Array<uint32_t, 5> PromotionFactor = {
            /** Pawn   **/ 0,
            /** Knight **/ 200000,
            /** Bishop **/ 50000 ,
            /** Rook   **/ 75000 ,
            /** Queen  **/ 300000
        };

        Move KillerOne;
        Move KillerTwo;

        Move TTMove;

        public:
        Policy(const Move kOne, const Move kTwo, const Move tt) : KillerOne(kOne), KillerTwo(kTwo), TTMove(tt) {}

        template<Piece Piece, enum Piece PromotionPiece = NAP>
        uint32_t Score(const Board& board, const HTable& history, const Move move) const
        {
            // Policy:
            //
            // The categories below give a rough idea of how the score is calculated for move ordering, but in practical
            // terms, the score isn't as categorical and there are overlaps between the categories (e.g. a good capture
            // can appear before a promotion):
            //
            // - Transposition Table Move
            // - Good Promotions (Queen or Knight)
            // - Good Captures
            // - Bad Promotions (Rook or Bishop)
            // - Good Quiet Moves
            //   - Killer Moves
            //   - Good History Moves
            // - Bad Captures
            // - Bad Quiet Moves
            //   - Bad History Moves

            if (move == TTMove) return MaximumScore;

            constexpr bool Promotion = PromotionPiece != NAP;

            const bool     capture = board[move.To()].Piece() != NAP;
            const bool goodCapture = capture ? SEE::Accurate(board, move, 0) : false;

            uint32_t score = ScoreAnchor;

            if (Promotion) score += PromotionFactor[PromotionPiece];

            if (CaptureOnly || capture) {
                score += MvvLva[board[move.To()].Piece()][Piece] * (goodCapture ? 20 : 1);

                return score;
            }

            if (move == KillerOne) score += HistoryLimit    ;
            if (move == KillerTwo) score += HistoryLimit / 2;

            score += history[Color][Piece][move.To()];

            return score;
        }

    };

} // StockDory

#endif //STOCKDORY_POLICY_H
