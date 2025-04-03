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

namespace StockDory
{

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

            constexpr PieceColor Get(const size_t idx) const
            {
                const size_t iIdx = idx / (sizeof(uint64_t) * 2);
                const size_t cIdx = idx % (sizeof(uint64_t) * 2);

                const uint64_t data = (Internal[iIdx] >> (cIdx * PieceColorSize)) & PieceColorMask;

                return PieceColor(static_cast<Piece>(data & PieceMask), static_cast<Color>(data >> ColorPos));
            }

            constexpr void Set(const size_t idx, const PieceColor pc)
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

        BitBoard         Occupancy;
        PackedPCArray  PieceColors;

        public:
        constexpr PackedPosition()
        {
            Occupancy   =       BBDefault;
            PieceColors = PackedPCArray();
        }

        constexpr PackedPosition(const Board& board)
        {
            Occupancy = board[White] | board[Black];

            auto iterator = BitBoardIterator(Occupancy);

            size_t i = 0;
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                PieceColors.Set(i, board[sq]);
                i++;
            }
        }

    };

} // StockDory

#endif //STOCKDORY_PACKEDPOSITION_H
