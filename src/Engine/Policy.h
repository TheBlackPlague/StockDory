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
#include "TunableParameter.h"

namespace StockDory
{

    template<Color Color, bool CaptureOnly = false>
    class Policy
    {

        // Reserving the byte for the index to be used for fast sorting
        constexpr static int32_t MaximumScore = std::numeric_limits<int32_t>::max() >> 8;

        constexpr static int32_t GoodCaptureBonus = 1 << 18;
        constexpr static int32_t   PromotionBonus = 1 << 19;

        constexpr static int32_t HistoryWeightScale = 128;

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

        const HTable& Counter ;
        const HTable& Followup;

        const CaptureHTable& CaptureHistory;

        const Move TTMove;

        public:
        Policy(
            const StockDory::Board& board,
            const Move kOne,
            const Move kTwo,
            const HTable&  history,
            const HTable&  counter,
            const HTable& followup,
            const CaptureHTable& captureHistory,
            const Move tt
        )
        : Board(board), KillerOne(kOne), KillerTwo(kTwo), History(history), Counter(counter),
          Followup(followup), CaptureHistory(captureHistory), TTMove(tt) {}

        template<Piece Piece, enum Piece PromotionPiece = NAP>
        int32_t Score(const Move move) const
        {
            if (move == TTMove) return MaximumScore;

            if (CaptureOnly || move.Capture()) {
                const auto targetPiece = move.EnPassant() ? Pawn : Board[move.To()].Piece();

                const uint16_t victimValue = SEE::PieceValue[targetPiece];

                const  int16_t captureHistory = CaptureHistory[Color][Piece][move.To()][targetPiece];

                const int32_t  victimScore = victimValue    * PolicyCaptureValueWeight                       ;
                const int32_t historyScore = captureHistory * PolicyCaptureHistoryWeight / HistoryWeightScale;

                const int32_t score = victimScore + historyScore;

                if (PromotionPiece != NAP) return PromotionFactor[PromotionPiece] * PromotionBonus + score;

                return score + (SEE::Accurate(Board, move, 0) ? GoodCaptureBonus : 0);
            }

            if (PromotionPiece != NAP) return PromotionFactor[PromotionPiece] * PromotionBonus;

            int32_t score = (
                  History [Color][Piece][move.To()] * PolicyHistoryWeight
                + Counter [Color][Piece][move.To()] * PolicyCounterWeight
                + Followup[Color][Piece][move.To()] * PolicyFollowupWeight
            ) / HistoryWeightScale;

            if (move.SameIdentity(KillerOne)) score += PolicyKillerOneBonus;
            if (move.SameIdentity(KillerTwo)) score += PolicyKillerTwoBonus;

            return score;
        }

    };

} // StockDory

#endif //STOCKDORY_POLICY_H
