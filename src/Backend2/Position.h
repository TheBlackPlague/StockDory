//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_POSITION_H
#define STOCKDORY_POSITION_H

#include "../External/strutil.h"

#include "Primitive/BitBoard.h"
#include "Primitive/Piece.h"
#include "Primitive/PositionProperty.h"
#include "Primitive/Zobrist.h"

namespace StockDory
{

    using PieceArray = Array<Piece, 64>;

    class FEN
    {

        constexpr static String Default = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

        String Internal;

        public:
        FEN() : FEN(Default) {}

        FEN(const String& str) : Internal(str) {}

        FEN(const PieceArray& pieceArray, const PositionProperty property)
        {
            OutputStringStream stream;

            s00 empty = 0;

            for (Square sq = A1; sq < InvalidSquare; ++sq) {
                if (sq % 8 == 0 && sq != A1) {
                    if (empty > 0) {
                        stream << static_cast<u32>(empty);
                        empty = 0;
                    }

                    stream << '/';
                }

                const Piece piece = pieceArray[sq];

                if (piece.Type() == InvalidPieceType) {
                    empty++;
                    continue;
                }

                if (empty > 0) stream << static_cast<u32>(empty);

                constexpr static Array<char, 7> PieceTypeChar { 'p', 'n', 'b', 'r', 'q', 'k', 'x' };

                char p = PieceTypeChar[piece.Type()];

                p = piece.Side() == White ? toupper(p) : p;

                stream << p;
            }

            stream << ' ';

            stream << property;
        }

        PieceArray ExtractPieceArray() const
        {
            PieceArray result {};

            InputStringStream stream (Internal);

            for (s00 rank = 7; rank < 8; rank--) {
                s00 file = 0;

                while (true) {
                    const char p = stream.get();

                    if (p == '/') break;

                    if (isdigit(p)) {
                        file += static_cast<s00>(p - 48);
                        continue;
                    }

                    const Side side = isupper(p) ? White : Black;

                    PieceType type;

                    switch (tolower(p)) {
                        case 'p': type =   Pawn; break;
                        case 'r': type =   Rook; break;
                        case 'n': type = Knight; break;
                        case 'b': type = Bishop; break;
                        case 'q': type =  Queen; break;
                        case 'k': type =   King; break;

                        default: std::unreachable();
                    }

                    const auto piece = Piece(side, type);

                    const auto sq = static_cast<Square>(rank * 8 + file);

                    result[sq] = piece;
                }
            }

            return result;
        }

        PositionProperty ExtractProperty() const
        {
            PositionProperty result {};

            const s00 idx = Internal.find(' ');

            const String fenProperty = Internal.substr(idx + 1);

            InputStringStream stream (fenProperty);

            stream >> result;

            return result;
        }

    };

    class Position
    {

        protected:
        Array<BitBoard, 3, 7> PieceBB {};
        Array<BitBoard, 3   >  SideBB {};

        PieceArray PieceArray {};

        PositionProperty Property {};

        Zobrist Zobrist {};

        public:
        Position(const FEN& fen)
        {
            PieceArray = fen.ExtractPieceArray();
            Property   = fen.ExtractProperty()  ;

            for (Square sq = A1; sq < InvalidSquare; ++sq) {
                Piece piece = PieceArray[sq];

                const auto type = piece.Type();
                const auto side = piece.Side();

                Set<true>(PieceBB[side][type], sq);
                Set<true>( SideBB[side]      , sq);

                Zobrist = ZobristHash(Zobrist, piece, sq);
            }

            Zobrist = Property.HashCastlingRights (Zobrist);
            Zobrist = Property.HashEnPassantSquare(Zobrist);

            if (Property.SideToMove() != White) Zobrist = ZobristHash(Zobrist);
        }

        FEN FEN() const { return StockDory::FEN(PieceArray, Property); }

        BitBoard operator [](const Piece piece) const { return operator[](piece.Type(), piece.Side()); }

        BitBoard operator [](const Side side) const
        {
            assert(side != InvalidSide, "Invalid side");

            return SideBB[side];
        }

        BitBoard operator [](const PieceType type, const Side side) const
        {
            assert(type != InvalidPieceType, "Invalid Piece Type");
            assert(side != InvalidSide     , "Invalid Side"      );

            return PieceBB[side][type];
        }

        StockDory::Zobrist Hash() const { return Zobrist; }

        PositionProperty PositionProperty() const { return Property; }

    };

} // StockDory

#endif //STOCKDORY_POSITION_H
