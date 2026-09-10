//
// Copyright (c) 2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

/*
 * The compact representation is based on chess-library:
 * https://github.com/Disservin/chess-library
 *
 * MIT License
 *
 * Copyright (c) 2023 Disservin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef STOCKDORY_PACKEDBOARD_H
#define STOCKDORY_PACKEDBOARD_H

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "Board.h"
#include "Misc.h"

namespace StockDory
{

    template<>
    class BasicBoard<BoardType::Packed>
    {

        // Eight occupancy bytes followed by up to 32 piece nibbles, in square order
        // Codes 12, 13, 14 and 15 carry en passant, castling and side-to-move state
        Array<uint8_t, 24> Internal {};

        constexpr static uint8_t CastlingMask(const Square sq, const Color color)
        {
            if (color == White) {
                if (sq == H1) return 0x8;
                if (sq == A1) return 0x4;
            } else if (color == Black) {
                if (sq == H8) return 0x2;
                if (sq == A8) return 0x1;
            }

            return 0;
        }

        constexpr uint8_t Code(const uint8_t index) const
        {
            return Internal[8 + index / 2] >> (index % 2 ? 0 : 4) & 0xF;
        }

        constexpr static PieceColor Decode(const uint8_t code, const Square sq)
        {
            if (code  < 12) return PieceColor(static_cast<::Piece>(code % 6), static_cast<Color>(code / 6));
            if (code == 12) return PieceColor(Pawn, sq < A5 ? White : Black);
            if (code == 13) return PieceColor(Rook, White);
            if (code == 14) return PieceColor(Rook, Black);

            return PieceColor(King, Black);
        }

        public:
        BasicBoard() : BasicBoard(BasicBoard<BoardType::Engine>()) {}

        explicit BasicBoard(const std::string& fen) : BasicBoard(BasicBoard<BoardType::Engine>(fen)) {}

        template<BoardType Other>
        explicit BasicBoard(const BasicBoard<Other>& board)
        {
            const BitBoard occ      = ~board[NAC];
            const Color    color    =  board.ColorToMove();
            const uint8_t  castling =  board.CastlingRights();
            const Square   ep       =  board.EnPassantSquare();

            if (Count(occ) > 32)
                throw std::invalid_argument("PackedBoard supports at most 32 pieces");

            if (color != White && color != Black)
                throw std::invalid_argument("PackedBoard requires a valid side to move");

            if (castling & ~0xF)
                throw std::invalid_argument("PackedBoard requires orthodox castling rights");

            if (ep != NASQ) {
                const Piece piece = board[ep].Piece();
                if (ep >= NASQ || (color == White ? ep < A6 || ep > H6 : ep < A3 || ep > H3) || piece != NAP)
                    throw std::invalid_argument("PackedBoard requires a valid en passant target");

                const PieceColor opposing = board[static_cast<Square>(ep ^ 8)];
                if (opposing.Piece() != Pawn || opposing.Color() != Opposite(color))
                    throw std::invalid_argument("PackedBoard requires the en passant pawn");
            }

            for (uint8_t i = 0; i < 8; i++) Internal[i] = occ >> (56 - i * 8);

            Array<uint8_t, 2> kings      {          };
            Array<Square , 2> kingSquare {NASQ, NASQ};

            uint8_t encodedCastling = 0;
            uint8_t index           = 0;

            BitBoardIterator iterator (occ);
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const PieceColor pc = board[sq];
                const Piece       p = pc.Piece();
                const Color       c = pc.Color();

                if (p >= NAP || c >= NAC)
                    throw std::invalid_argument("PackedBoard requires valid occupied squares");

                uint8_t code = p + c * 6;

                if (p == King) {
                    kings     [c]   ++;
                    kingSquare[c] = sq;

                    if (c == Black && color == Black) code = 15;
                } else if (p == Pawn && ep != NASQ && sq == static_cast<Square>(ep ^ 8)) {
                    code = 12;
                } else if (p == Rook) {
                    if (const uint8_t right = CastlingMask(sq, c) & castling) {
                        encodedCastling |= right;
                        code = c == White ? 13 : 14;
                    }
                }

                Internal[8 + index / 2] |= code << (index % 2 ? 0 : 4);
                index++;
            }

            if (kings[White] != 1 || kings[Black] != 1)
                throw std::invalid_argument("PackedBoard requires one king of each color");

            if (castling != encodedCastling || (castling & 0xC && kingSquare[White] != E1) ||
                                               (castling & 0x3 && kingSquare[Black] != E8))
                throw std::invalid_argument(
                    "PackedBoard requires castling kings and rooks on their home squares"
                );
        }

        [[nodiscard]]
        constexpr BitBoard Occupied() const
        {
            BitBoard occupied = BBDefault;

            for (uint8_t i = 0; i < 8; i++) occupied = occupied << 8 | Internal[i];

            return occupied;
        }

        [[nodiscard]]
        constexpr PieceColor operator [](const Square sq) const
        {
            assert(sq < NASQ);

            const BitBoard occupied = Occupied();
            const BitBoard square   = FromSquare(sq);

            if (!(occupied & square)) return PieceColor(NAP, NAC);

            return Decode(Code(Count(occupied & (square - 1))), sq);
        }

        [[nodiscard]]
        constexpr BitBoard operator [](const Color color) const
        {
            const BitBoard occupied = Occupied();

            if (color == NAC) return ~occupied;

            BitBoard result = BBDefault;
            uint8_t  index  =         0;

            BitBoardIterator iterator (occupied);
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                if (Decode(Code(index), sq).Color() == color) result |= FromSquare(sq);
                index++;
            }

            return result;
        }

        [[nodiscard]]
        constexpr BitBoard PieceBoard(const Piece piece, const Color color) const
        {
            assert(piece < NAP);
            assert(color < NAC);

            BitBoard result = BBDefault;
            uint8_t  index  =         0;

            BitBoardIterator iterator (Occupied());
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const PieceColor pc = Decode(Code(index), sq);

                if (pc.Piece() == piece && pc.Color() == color) result |= FromSquare(sq);

                index++;
            }

            return result;
        }

        template<Color Color>
        [[nodiscard]]
        constexpr BitBoard PieceBoard(const Piece piece) const
        {
            return PieceBoard(piece, Color);
        }

        [[nodiscard]]
        constexpr Color ColorToMove() const
        {
            for (uint8_t i = 8; i < Internal.size(); i++)
                if ((Internal[i]  & 0xF) == 15 ||
                     Internal[i] >>   4  == 15  ) return Black;

            return White;
        }

        [[nodiscard]]
        constexpr uint8_t CastlingRights() const
        {
            uint8_t result = 0;
            uint8_t index  = 0;

            BitBoardIterator iterator (Occupied());
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const uint8_t code = Code(index);

                if (code == 13 || code == 14) result |= CastlingMask(sq, code == 13 ? White : Black);

                index++;
            }

            return result;
        }

        template<Color Color>
        [[nodiscard]]
        constexpr bool CastlingRightK() const
        {
            static_assert(Color == White || Color == Black);

            return CastlingRights() & (Color == White ? 0x8 : 0x2);
        }

        template<Color Color>
        [[nodiscard]]
        constexpr bool CastlingRightQ() const
        {
            static_assert(Color == White || Color == Black);

            return CastlingRights() & (Color == White ? 0x4 : 0x1);
        }

        [[nodiscard]]
        constexpr Square EnPassantSquare() const
        {
            uint8_t index = 0;

            BitBoardIterator iterator (Occupied());
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                if (Code(index) == 12) return static_cast<Square>(sq ^ 8);

                index++;
            }

            return NASQ;
        }

        [[nodiscard]]
        constexpr BitBoard EnPassant() const
        {
            const Square sq = EnPassantSquare();
            return sq == NASQ ? BBDefault : FromSquare(sq);
        }

        [[nodiscard]]
        constexpr const Array<uint8_t, 24>& Data() const
        {
            return Internal;
        }

        [[nodiscard]]
        std::string Fen() const
        {
            return BasicBoard<BoardType::Engine>(*this).Fen();
        }

        [[nodiscard]]
        constexpr bool operator ==(const BasicBoard& other) const = default;

    };

    using PackedBoard = BasicBoard<BoardType::Packed>;

    static_assert(sizeof(PackedBoard) == 24);
    static_assert(std::is_trivially_copyable_v<PackedBoard>);

} // StockDory

#endif //STOCKDORY_PACKEDBOARD_H
