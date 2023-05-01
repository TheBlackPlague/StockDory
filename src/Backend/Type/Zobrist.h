//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_ZOBRIST_H
#define STOCKDORY_ZOBRIST_H

#include <cstdint>
#include <array>

#include "Piece.h"
#include "Color.h"
#include "Square.h"

#include "../Template/MoveType.h"

using ZobristHash = uint64_t;

namespace StockDory::Zobrist
{

    class RNG
    {

        private:
            ZobristHash Seed = 0x7F6EAD4C3B2A1908;

        public:
            [[nodiscard]]
            constexpr inline ZobristHash Next()
            {
                Seed ^= Seed >> 12;
                Seed ^= Seed << 25;
                Seed ^= Seed >> 27;

                return Seed * 0x2545F4914F6CDD1DULL;
            }

    };

    class ZobristKeyTable
    {

        public:
            std::array<std::array<std::array<ZobristHash, 64>, 7>, 3> PieceKey {};

            std::array<ZobristHash, 16>  CastlingKey {};
            std::array<ZobristHash, 65> EnPassantKey {};

            ZobristHash ColorToMoveKey;

            constexpr ZobristKeyTable()
            {
                RNG rng = RNG();

                for (size_t c = 0; c < 3; c++) for (size_t p = 0; p < 7; p++) for (size_t sq = 0; sq < 64; sq++) {
                    if (c > 1 || p > 5) PieceKey[c][p][sq] = 0         ;
                    else                PieceKey[c][p][sq] = rng.Next();
                }

                for (size_t i = 0; i < 16; i++) {
                    CastlingKey[i] = 0;

                    BitBoardIterator iterator (i);
                    for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                        ZobristHash key = CastlingKey[1ULL << sq];
                        CastlingKey[i] ^= key ? key : rng.Next();
                    }
                }

                for (size_t i = 0; i < 65; i++) {
                    if (i == 64) EnPassantKey[i] = 0         ;
                    else         EnPassantKey[i] = rng.Next();
                }

                ColorToMoveKey = rng.Next();
            }

    };

}

constexpr StockDory::Zobrist::ZobristKeyTable ZobristKeyTable;

template<MoveType T>
constexpr inline ZobristHash HashPiece(const ZobristHash hash, const Piece p, const Color c, const Square sq)
{
    if (T & ZOBRIST) return hash ^ ZobristKeyTable.PieceKey[c][p][sq];
    return hash;
}

template<MoveType T>
constexpr inline ZobristHash HashCastling(const ZobristHash hash, const uint8_t castlingRight)
{
    if (T & ZOBRIST) return hash ^ ZobristKeyTable.CastlingKey[castlingRight];
    return hash;
}

template<MoveType T>
constexpr inline ZobristHash HashEnPassant(const ZobristHash hash, const Square enPassantSquare)
{
    if (T & ZOBRIST) return hash ^ ZobristKeyTable.EnPassantKey[enPassantSquare];
    return hash;
}

template<MoveType T>
constexpr inline ZobristHash HashColorFlip(const ZobristHash hash)
{
    if (T & ZOBRIST) return hash ^ ZobristKeyTable.ColorToMoveKey;
    return hash;
}

#endif //STOCKDORY_ZOBRIST_H
