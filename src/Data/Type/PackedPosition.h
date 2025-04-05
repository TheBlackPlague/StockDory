//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PACKEDPOSITION_H
#define STOCKDORY_PACKEDPOSITION_H

#include <array>

#include "../../Backend/Board.h"
#include "../../Backend/Type/BitBoard.h"
#include "../../Backend/Type/Piece.h"
#include "../../Backend/Type/PieceColor.h"

#include "../Format/GranaVersion.h"

namespace StockDory
{

    template<GranaVersion Version>
    struct PackedPosition
    {

        private:
        struct PackedPCArray
        {

            private:
            constexpr static uint8_t PieceColorSize =   4;
            constexpr static uint8_t PieceColorMask = 0xF;
            constexpr static uint8_t      PieceMask = 0x7;
            constexpr static uint8_t      ColorPos  =   3;

            std::array<uint64_t, 2> Internal;

            public:
            constexpr PackedPCArray()
            {
                Internal = {};
            }

            constexpr inline PieceColor Get(const size_t idx) const
            {
                const size_t iIdx = idx / (sizeof(uint64_t) * 2);
                const size_t cIdx = idx % (sizeof(uint64_t) * 2);

                const uint64_t data = (Internal[iIdx] >> (cIdx * PieceColorSize)) & PieceColorMask;

                return PieceColor(static_cast<Piece>(data & PieceMask), static_cast<Color>(data >> ColorPos));
            }

            constexpr inline void Set(const size_t idx, const PieceColor pc)
            {
                const size_t iIdx = idx / (sizeof(uint64_t) * 2);
                const size_t cIdx = idx % (sizeof(uint64_t) * 2);

                const Piece piece = pc.Piece();
                const Color color = pc.Color();

                const size_t shift = cIdx * PieceColorSize;

                Internal[iIdx] &= std::rotl(~static_cast<uint64_t>(PieceColorMask), shift);
                Internal[iIdx] |= (piece | (color << ColorPos))                  << shift ;
            }

        };

        constexpr static size_t BoardOffset0 = 0;

        BitBoard      Internal0;
        PackedPCArray Internal1;

        public:
        constexpr PackedPosition()
        {
            Internal0 =       BBDefault;
            Internal1 = PackedPCArray();
        }

        constexpr PackedPosition(const Board& board)
        {
            Internal0 = board[White] | board[Black];

            auto iterator = BitBoardIterator(Internal0);

            size_t i = 0;
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) Internal1.Set(i++, board[sq]);
        }

        constexpr inline void LoadBoard(Board& board) const
        {
            auto iterator = BitBoardIterator(Internal0);

            size_t i = 0;
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                PieceColor pc = Internal1.Get(i++);

                board.InsertNative(pc.Piece(), pc.Color(), sq);
            }
        }

    };

} // StockDory

#endif //STOCKDORY_PACKEDPOSITION_H
