//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEED_H
#define STOCKDORY_SEED_H

#include "PackedPosition.h"
#include "Result.h"
#include "Stalk.h"

namespace StockDory
{

    template<GranaVersion Version>
    struct Seed
    {

        private:
        using PackedPosition = PackedPosition<Version>;
        using Stalk          = Stalk         <Version>;

        struct RemainingData
        {

            private:
            constexpr static uint8_t             ResultMask = 0x3;
            constexpr static uint8_t RemainingStalkCountPos =   2;

            // Offset -> CastlingRightAndColorToMove
            constexpr static size_t BoardOffset0 = 256;

            uint8_t Internal0;
            uint8_t Internal1;

            std::array<uint8_t, 2> Reserved = {};

            Stalk   Internal2;

            public:
            constexpr RemainingData()
            {
                Internal0 =       0;
                Internal1 =       0;
                Internal2 = Stalk();
            }

            constexpr RemainingData(const Board&    board,
                                    const Result    result,
                                    const uint8_t   remainingStalkCount,
                                    const Stalk     stalk)
            {
                std::memcpy(
                    &Internal0,
                    reinterpret_cast<const uint8_t*>(&board) + BoardOffset0,
                    sizeof(uint8_t)
                );

                Internal1 = result | remainingStalkCount << RemainingStalkCountPos;
                Internal2 =                                                  stalk;
            }

            constexpr inline void LoadBoard(Board& board) const
            {
                std::memcpy(
                    reinterpret_cast<uint8_t*>(&board) + BoardOffset0,
                    &Internal0,
                    sizeof(uint8_t)
                );
            }

            constexpr inline Result Result() const
            {
                return static_cast<enum Result>(Internal1 & ResultMask);
            }

            constexpr inline uint8_t RemainingStalkCount() const
            {
                return Internal1 >> RemainingStalkCountPos;
            }

            constexpr inline Stalk Initial() const
            {
                return Internal2;
            }

            constexpr inline uint8_t PostIncrementRemainingStalkCount()
            {
                const uint8_t v = RemainingStalkCount();

                Internal1 &= ResultMask;
                Internal1 |= (v + 1) << RemainingStalkCountPos;

                return v;
            }

        };

        PackedPosition Internal0;
        RemainingData  Internal1;

        public:
        constexpr Seed()
        {
            Internal0 = PackedPosition();
            Internal1 =  RemainingData();
        }

        constexpr Seed(const Board& board, const Result result, const uint8_t remainingStalkCount,
                       const Stalk  stalk)
        {
            Internal0 = PackedPosition(board                                    );
            Internal1 =  RemainingData(board, result, remainingStalkCount, stalk);
        }

        constexpr inline void LoadBoard(Board& board) const
        {
            std::memset(&board, 0, sizeof(Board));

            Internal0.LoadBoard(board);
            Internal1.LoadBoard(board);
        }

        constexpr inline PackedPosition Position() const
        {
            return Internal0;
        }

        constexpr inline Result Result() const
        {
            return Internal1.Result();
        }

        constexpr inline uint8_t RemainingStalkCount() const
        {
            return Internal1.RemainingStalkCount();
        }

        constexpr inline Stalk Initial() const
        {
            return Internal1.Initial();
        }

        constexpr inline uint8_t PostIncrementRemainingStalkCount()
        {
            return Internal1.PostIncrementRemainingStalkCount();
        }

    };

} // StockDory

#endif //STOCKDORY_SEED_H
