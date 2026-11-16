//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_MOVELIST_H
#define STOCKDORY_MOVELIST_H

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>

#include "MoveGenerator.h"
#include "Policy.h"

namespace StockDory
{

    template<Side Us, Ordering Ordering = Unordered, MoveSet Set = AllMoves>
    class MoveList
    {

        using OrderingPolicy = Policy<Us, Set == CaptureMoves>;

        struct OrderedMove
        {

            int32_t Score;
            Move    Move ;

        };

        using Entry = std::conditional_t<Ordering == Ordered, OrderedMove, Move>;

        Array<Entry, MaxMove> Internal;

        uint8_t Size = 0;

        public:
        struct IndexedMove
        {

            uint8_t Index;
            Move    Move ;

            // ReSharper disable once CppNonExplicitConversionOperator
            operator StockDory::Move() const { return Move; }

        };

        class Iterator
        {

            MoveList& List;

            uint8_t Index = 0;

            public:
            explicit Iterator(MoveList& list) : List(list) {}

            bool operator !=(const std::default_sentinel_t) const { return Index < List.Size; }

            IndexedMove operator *() const { return { .Index = Index, .Move = List.Next(Index) }; }

            Iterator& operator ++() { Index++; return *this; }

        };

        explicit MoveList(const Board& board) requires (Ordering == Unordered)
        {
            Emitter emitter (*this);

            Generate<Us, Set>(board, emitter);
        }

        explicit MoveList(
            const Board&                  board,
            const uint8_t                   ply,
            const KTable&                killer,
            const HTable&               history,
            const HTable&               counter,
            const HTable&              followup,
            const CaptureHTable& captureHistory,
            const Move                   ttMove = {}
        ) requires (Ordering == Ordered)
        {
            const OrderingPolicy policy (
                board, killer[0][ply], killer[1][ply], history, counter, followup, captureHistory, ttMove
            );

            Emitter emitter (*this, policy);

            Generate<Us, Set>(board, emitter);
        }

        uint8_t Count() const { return Size; }

        Move operator [](const uint8_t index) const requires (Ordering == Unordered) { return Internal[index]; }

        const Move* begin() const requires (Ordering == Unordered) { return Internal.data()       ; }
        const Move* end  () const requires (Ordering == Unordered) { return Internal.data() + Size; }

        Iterator begin() requires (Ordering == Ordered) { return Iterator(*this); }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        std::default_sentinel_t end() const requires (Ordering == Ordered) { return {}; }

        private:
        Move Next(const uint8_t sorted) requires (Ordering == Ordered)
        {
            int32_t best = Internal[sorted].Score << 8 | (MaxMove - sorted);
            for (uint8_t i = sorted + 1; i < Size; i++) best = std::max(best, Internal[i].Score << 8 | (MaxMove - i));

            const uint8_t index = MaxMove - (best & 0xFF);

            std::swap(Internal[index], Internal[sorted]);

            return Internal[sorted].Move;
        }

        class Emitter
        {

            MoveList& List;

            Entry* Cursor;

            const OrderingPolicy* Policy = nullptr;

            public:
            explicit Emitter(MoveList& list) requires (Ordering == Unordered)
            : List(list), Cursor(list.Internal.data()) {}

            Emitter(MoveList& list, const OrderingPolicy& policy)
            : List(list), Cursor(list.Internal.data()), Policy(&policy) {}

            ~Emitter() { List.Size = static_cast<uint8_t>(Cursor - List.Internal.data()); }

            [[clang::always_inline]]
            void PawnMoves(const PawnTargets& targets)
            {
                const BitBoard movers = Shift<Down    <Us>>(                targets.Single ) |
                                        Shift<Down    <Us>>(Shift<Down<Us>>(targets.Double)) |
                                        Shift<DownEast<Us>>(                targets.West   ) |
                                        Shift<DownWest<Us>>(                targets.East   ) |
                                        targets.EnPassantFrom                                ;

                for (const Square from : Squares(movers)) {
                    const BitBoard pawn = FromSquare(from);

                    const BitBoard captures = Shift<UpWest<Us>>(pawn) & targets.West |
                                              Shift<UpEast<Us>>(pawn) & targets.East ;

                    const BitBoard single = Shift<Up<Us>>(              pawn ) & targets.Single;
                    const BitBoard twice  = Shift<Up<Us>>(Shift<Up<Us>>(pawn)) & targets.Double;

                    const BitBoard passant = Get(targets.EnPassantFrom, from) ?
                                             FromSquare(targets.EnPassantTo) : BBDefault;

                    for (const Square to : Squares(captures | single | twice | passant)) {
                        const bool capture = Get(captures, to);

                             if (Get(LastRank<Us>   , to)) EmitPromotions(from, to, capture);

                        else if (Get(passant        , to)) Emit<Pawn>(from, to, MoveFlag::EnPassant );
                        else if (Get(twice          , to)) Emit<Pawn>(from, to, MoveFlag::DoublePush);
                        else if (capture                 ) Emit<Pawn>(from, to, MoveFlag::Capture   );
                        else                               Emit<Pawn>(from, to, MoveFlag::Base      );
                    }
                }
            }

            template<PieceType Type>
            [[clang::always_inline]]
            void PieceMoves(const Square from, const BitBoard quiets, const BitBoard captures)
            {
                for (const Square to : Squares(quiets | captures))
                    Emit<Type>(from, to, Get(captures, to) ? MoveFlag::Capture : MoveFlag::Base);
            }

            [[clang::always_inline]]
            void KingMoves(const Square from, const BitBoard quiets, const BitBoard captures, const BitBoard castles)
            {
                for (const Square to : Squares(quiets | captures | castles)) {
                         if (Get(castles , to)) EmitCastle(from, to);

                    else if (Get(captures, to)) Emit<King>(from, to, MoveFlag::Capture);
                    else                        Emit<King>(from, to, MoveFlag::Base   );
                }
            }

            private:
            template<PieceType Type, PieceType Promotion = NAPT>
            [[clang::always_inline]]
            void Emit(const Square from, const Square to, const MoveFlag flag)
            {
                const Move move (from, to, flag);

                if constexpr (Ordering == Ordered) *Cursor++ = { Policy->template Score<Type, Promotion>(move), move };
                else                               *Cursor++ = move;
            }

            [[clang::always_inline]]
            void EmitPromotions(const Square from, const Square to, const bool capture)
            {
                Emit<Pawn, Queen >(from, to, capture ? MoveFlag::QueenCapturePromotion  : MoveFlag::QueenPromotion );
                Emit<Pawn, Knight>(from, to, capture ? MoveFlag::KnightCapturePromotion : MoveFlag::KnightPromotion);
                Emit<Pawn, Rook  >(from, to, capture ? MoveFlag::RookCapturePromotion   : MoveFlag::RookPromotion  );
                Emit<Pawn, Bishop>(from, to, capture ? MoveFlag::BishopCapturePromotion : MoveFlag::BishopPromotion);
            }

            [[clang::always_inline]]
            void EmitCastle(const Square from, const Square rookFrom)
            {
                if (rookFrom > from) Emit<King>(from, MakeSquare(6, RankOf(from)), MoveFlag::KingCastle );
                else                 Emit<King>(from, MakeSquare(2, RankOf(from)), MoveFlag::QueenCastle);
            }

        };

    };

} // StockDory

#endif //STOCKDORY_MOVELIST_H
