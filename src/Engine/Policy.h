//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
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

        // Reserving the 8 upper bits for the index to be used for fast sorting
        constexpr static int32_t MaximumScore = std::numeric_limits<int32_t>::max() >> 8;

        constexpr static int32_t PromotionMultiplier = 100000;

        constexpr static Array<uint8_t, 5> PromotionFactor = {
            0, //   Pawn
            3, // Knight
            1, // Bishop
            2, //   Rook
            4  //  Queen
        };

        const Board& Board;

        const Move KillerOne;
        const Move KillerTwo;

        const HTable& History;

        const HTable& Continuation;

        const Move TTMove;

        public:
        Policy(
            const StockDory::Board& board,
            const Move kOne,
            const Move kTwo,
            const HTable&      history,
            const HTable& continuation,
            const Move tt
        )
        : Board(board), KillerOne(kOne), KillerTwo(kTwo), History(history), Continuation(continuation), TTMove(tt) {}

        template<Piece Piece, enum Piece PromotionPiece = NAP>
        int32_t Score(const Move move) const
        {
            // Policy:
            //
            // The categories below give a rough idea of how the score is calculated for move ordering, but in practical
            // terms, the score isn't as categorical and there are overlaps between the categories (e.g. a good capture
            // can appear before a promotion):
            //
            // - Transposition Table Move
            // - Promotions
            // - Good Captures (SEE >= 0)
            // - Good Quiet Moves
            //   - Killer Moves
            //   - Good History Moves
            // - Bad Captures (SEE < 0)
            // - Bad Quiet Moves
            //   - Bad History Moves

            if (move == TTMove) return MaximumScore;

            int32_t score = 0;

            if (PromotionPiece != NAP) score += PromotionFactor[PromotionPiece] * PromotionMultiplier;

            if (CaptureOnly || move.Capture()) {
                const bool goodCapture = SEE::Accurate(Board, move, 0);
                score += MvvLva[move.EnPassant() ? Pawn : Board[move.To()].Piece()][Piece] * (goodCapture ? 20 : 1);

                return score;
            }

            if (move.SameIdentity(KillerOne)) score += HistoryLimit    ;
            if (move.SameIdentity(KillerTwo)) score += HistoryLimit / 2;

            score += History[Color][Piece][move.To()];

            if (PromotionPiece == NAP) score += Continuation[Color][Piece][move.To()];

            return score;
        }

    };

} // StockDory

#endif //STOCKDORY_POLICY_H
