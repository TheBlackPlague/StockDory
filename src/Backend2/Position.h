//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_POSITION_H
#define STOCKDORY_POSITION_H

#include <cassert>

#include "../External/strutil.h"

#include "BitBoard.h"
#include "Piece.h"
#include "PositionProperty.h"
#include "Zobrist.h"

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

            usize empty = 0;

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

            for (usize rank = 7; rank < 8; rank--) {
                usize file = 0;

                while (true) {
                    const char p = stream.get();

                    if (p == '/') break;

                    if (isdigit(p)) {
                        file += static_cast<usize>(p - 48);
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

            const usize idx = Internal.find(' ');

            const String fenProperty = Internal.substr(idx + 1);

            InputStringStream stream (fenProperty);

            stream >> result;

            return result;
        }

    };

    class DefaultEvaluationHandler
    {

        static void Reset(const TID _ = 0) {};

        [[clang::always_inline]]
        static void  PreMakeMove(const TID threadId = 0) {};
        [[clang::always_inline]]
        static void PostMakeMove(const TID threadId = 0) {};

        [[clang::always_inline]]
        static void  PreUndoMove(const TID threadId = 0) {};
        [[clang::always_inline]]
        static void PostUndoMove(const TID threadId = 0) {};

        static void   Activate(const Side side, const PieceType type, const Square sq, const TID threadId = 0) {};
        static void Deactivate(const Side side, const PieceType type, const Square sq, const TID threadId = 0) {};

        static void Transition(const Side side, const PieceType type, const Square origin, const Square target,
                               const TID threadId = 0) {};

        static Score Evaluate(const Side perspective, const TID threadId = 0) { std::unreachable(); }

    };

    template<class EvaluationHandler = DefaultEvaluationHandler>
    class Position
    {

        protected:
        Zobrist Zobrist {};

        Array<BitBoard, 3, 7> PieceBB {};
        Array<BitBoard, 3   >  SideBB {};

        PieceArray PieceArray {};

        PositionProperty Property {};

        public:
        explicit Position(const FEN& fen)
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

        // ReSharper disable once CppNonExplicitConversionOperator
        operator FEN() const { return FEN(PieceArray, Property); }

        Score Evaluate(const TID threadId = 0)
        { return EvaluationHandler::Evaluate(Property.SideToMove(), threadId); }

        BitBoard operator [](const Piece piece) const { return operator[](piece.Side(), piece.Type()); }

        BitBoard operator [](const Side side) const
        {
            assert(side != InvalidSide, "Invalid side");

            return SideBB[side];
        }

        BitBoard operator [](const Side side, const PieceType type) const
        {
            assert(type != InvalidPieceType, "Invalid Piece Type");
            assert(side != InvalidSide     , "Invalid Side"      );

            return PieceBB[side][type];
        }

        StockDory::Zobrist Hash() const { return Zobrist; }

        PositionProperty PositionProperty() const { return Property; }

        template<Side Side>
        bool UnderCheck() const
        {
            static_assert(Side != InvalidSide, "Invalid side");

            const Square king = AsSquare(PieceBB[Side][King]);

            if (Attack::Pawn  [Side][king] & PieceBB[~Side][Pawn  ]) return true;
            if (Attack::Knight      [king] & PieceBB[~Side][Knight]) return true;

            // TODO: Replace usage of InvalidSide BB
            const BitBoard occ = ~SideBB[InvalidSide];

            if (Attack::Sliding<Bishop>(king, occ) & (PieceBB[~Side][Bishop] | PieceBB[~Side][Queen]))
                return true;

            if (Attack::Sliding< Rook >(king, occ) & (PieceBB[~Side][ Rook ] | PieceBB[~Side][Queen]))
                return true;

            return false;
        }

        template<Side Side>
        CheckBitBoard CheckBB() const
        {
            u08           count {};
            CheckBitBoard check {};

            const Square king = AsSquare(PieceBB[Side][King]);

            const BitBoard   pawnCheck = Attack::Pawn  [Side][king] & PieceBB[~Side][Pawn  ];
            const BitBoard knightCheck = Attack::Knight      [king] & PieceBB[~Side][Knight];

            check.Mask |=   pawnCheck;
            check.Mask |= knightCheck;

            if (  pawnCheck) count++;
            if (knightCheck) count++;

            // TODO: Replace usage of InvalidSide BB
            const BitBoard occ = ~SideBB[InvalidSide];

            const BitBoard diagonalCheck = Attack::Sliding<Bishop>(king, occ) &
                (PieceBB[~Side][Bishop] | PieceBB[~Side][Queen]);

            const BitBoard straightCheck = Attack::Sliding< Rook >(king, occ) &
                (PieceBB[~Side][ Rook ] | PieceBB[~Side][Queen]);

            if (diagonalCheck) {
                const Square checkSq = AsSquare(diagonalCheck);
                check.Mask |= Ray::Between[king][checkSq];
                check.Mask |= diagonalCheck;

                count++;
            }

            if (straightCheck) {
                const Square checkSq = AsSquare(straightCheck);
                check.Mask |= Ray::Between[king][checkSq];
                check.Mask |= straightCheck;

                count++;

                if (Count(straightCheck) > 1) count++;
            }

            if (check.Mask == 0) check.Mask = ~check.Mask;

            check.Double = count > 1;

            return check;
        }

        template<Side Side>
        PinBitBoard PinBB() const
        {
            PinBitBoard pin {};

            const Square king = AsSquare(PieceBB[Side][King]);

            const BitBoard occ = SideBB[~Side];

            const BitBoard diagonalCheck = Attack::Sliding<Bishop>(king, occ) &
                (PieceBB[~Side][Bishop] | PieceBB[~Side][Queen]);

            const BitBoard straightCheck = Attack::Sliding< Rook >(king, occ) &
                (PieceBB[~Side][ Rook ] | PieceBB[~Side][Queen]);

            for (const Square checkSq : diagonalCheck) {
                const BitBoard possiblePin = Ray::Between[king][checkSq] | AsBitBoard(checkSq);

                if (Count(possiblePin & SideBB[Side]) == 1) pin.DiagonalMask |= possiblePin;
            }

            for (const Square checkSq : straightCheck) {
                const BitBoard possiblePin = Ray::Between[king][checkSq] | AsBitBoard(checkSq);

                if (Count(possiblePin & SideBB[Side]) == 1) pin.StraightMask |= possiblePin;
            }

            return pin;
        }

    };

} // StockDory

#endif //STOCKDORY_POSITION_H
