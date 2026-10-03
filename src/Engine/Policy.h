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

        constexpr static int32_t GoodTacticalBonus = HistoryLimit * 3;

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

        const CaptureHTable& CaptureHistory;

        const Move TTMove;

        public:
        Policy(
            const StockDory::Board& board,
            const Move kOne,
            const Move kTwo,
            const HTable&      history,
            const HTable& continuation,
            const CaptureHTable& captureHistory,
            const Move tt
        )
        : Board(board), KillerOne(kOne), KillerTwo(kTwo), History(history), Continuation(continuation),
          CaptureHistory(captureHistory), TTMove(tt) {}

        template<Piece Piece, enum Piece PromotionPiece = NAP>
        int32_t Score(const Move move) const
        {

            // Policy:
            //
            // The transposition table move always comes first, followed by promotions and good captures. Following them
            // are good quiets, which are then followed by bad captures interleaved with moderately-favoured quiets. At
            // last, there are strongly disfavoured quiets.
            //
            // Score Range(s)      | Move Types
            // 8388607             | Transposition Table Move
            // 449152 ... 585636   | Queen Promotions / Capturing Knight Underpromotions
            // 349152 ... 449151   | Queen/Knight Promotions / Capturing Rook Underpromotions
            // 249152 ... 349151   | Knight/Rook Underpromotions / Capturing Bishop Underpromotions
            // 149152 ... 249151   | Rook/Bishop Underpromotions / Higher-Scoring Good Captures
            //  72768 ... 149151   | Quiet Bishop Underpromotions / Remaining Good Captures
            //  21386 ...  49152   | Strongly Favoured Quiet Moves
            // -14383 ...  21385   | Bad Captures / Similarly Scored Quiets
            // -32768 ... -14384   | Strongly Disfavoured Quiet Moves
            //
            // Capturing underpromotions refer to moves that capture a piece while promoting to piece of lesser value
            // than the one they're capturing; MvvLva (Most Valuable Victim Least Valuable Attacker) combined with SEE
            // (static exchange evaluation) typically scores these well since they are usually good captures.
            //
            // Good & bad captures refer to captures accepted or rejected by SEE and not their capture-history scores,
            // however within a category of captures, they are prioritized by their capture-history scores

            if (move == TTMove) return MaximumScore;

            int32_t score = 0;

            if (PromotionPiece != NAP)
                score += GoodTacticalBonus + PromotionFactor[PromotionPiece] * PromotionMultiplier;

            if (CaptureOnly || move.Capture()) {
                const bool goodCapture = SEE::Accurate(Board, move, 0);
                const auto capturedPiece = move.EnPassant() ? Pawn : Board[move.To()].Piece();

                score += MvvLva[capturedPiece][Piece] * (goodCapture ? 20 : 1);
                score += CaptureHistory[Color][Piece][move.To()][capturedPiece];

                if (PromotionPiece == NAP && goodCapture) score += GoodTacticalBonus;

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
