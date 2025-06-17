//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_POSITIONPROPERTY_H
#define STOCKDORY_POSITIONPROPERTY_H

#include "Base.h"
#include "Side.h"
#include "Zobrist.h"

namespace StockDory
{

    enum CastlingDirection : u08 { K, Q, InvalidCastlingDirection };

    class PositionProperty
    {

        constexpr static u08 CastlingRightMask = 0b1111;

        constexpr static u08 WhiteKSideCastlingRightMask = 0b1000;
        constexpr static u08 WhiteQSideCastlingRightMask = 0b0100;
        constexpr static u08 BlackKSideCastlingRightMask = 0b0010;
        constexpr static u08 BlackQSideCastlingRightMask = 0b0001;

        constexpr static u08 SideToMoveMask  = 0b00010000;
        constexpr static u08 SideToMoveShift = 4;

        // [ SIDE TO MOVE ] [ WHITE K CR ] [ WHITE Q CR ] [ BLACK K CR ] [ BLACK Q CR ]
        u08    Internal0 = White << SideToMoveShift | WhiteKSideCastlingRightMask | WhiteQSideCastlingRightMask
                                                    | BlackKSideCastlingRightMask | BlackQSideCastlingRightMask;

        // [ EN PASSANT SQUARE ]
        Square Internal1 = InvalidSquare;

        // [ HALF-MOVE CLOCK ]
        u08    Internal2 = 0;

        // [ FULL-MOVE NUMBER ]
        u16    Internal3 = 1;

        public:
        Zobrist HashCastlingRights(const Zobrist hash) const
        {
            return ZobristHash(hash, Internal0 & CastlingRightMask);
        }

        Zobrist HashEnPassantSquare(const Zobrist hash) const { return ZobristHash(hash, Internal1); }

        template<Side Side>
        bool CanCastle() const
        {
            static_assert(Side == White || Side == Black, "Invalid Side");

            if (Side == White) return Internal0 & (WhiteKSideCastlingRightMask | WhiteQSideCastlingRightMask);
            if (Side == Black) return Internal0 & (BlackKSideCastlingRightMask | BlackQSideCastlingRightMask);

            std::unreachable();
        }

        template<Side Side, CastlingDirection Direction>
        bool CanCastle() const
        {
            static_assert(Side      == White || Side      == Black, "Invalid Side"              );
            static_assert(Direction == K     || Direction == Q    , "Invalid Castling Direction");

            if (Side == White) {
                if (Direction == K) return Internal0 & WhiteKSideCastlingRightMask;
                if (Direction == Q) return Internal0 & WhiteQSideCastlingRightMask;
            }

            if (Side == Black) {
                if (Direction == K) return Internal0 & BlackKSideCastlingRightMask;
                if (Direction == Q) return Internal0 & BlackQSideCastlingRightMask;
            }

            std::unreachable();
        }

        template<Side Side>
        void InvalidateCastlingRights()
        {
            static_assert(Side == White || Side == Black, "Invalid Side");

            if (Side == White) Internal0 &= ~(WhiteKSideCastlingRightMask | WhiteQSideCastlingRightMask);
            if (Side == Black) Internal0 &= ~(BlackKSideCastlingRightMask | BlackQSideCastlingRightMask);

            std::unreachable();
        }

        template<Side Side, CastlingDirection Direction>
        void InvalidateCastlingRights()
        {
            static_assert(Side      == White || Side      == Black, "Invalid Side"              );
            static_assert(Direction == K     || Direction == Q    , "Invalid Castling Direction");

            if (Side == White) {
                if (Direction == K) Internal0 &= ~WhiteKSideCastlingRightMask;
                if (Direction == Q) Internal0 &= ~WhiteQSideCastlingRightMask;
            }

            if (Side == Black) {
                if (Direction == K) Internal0 &= ~BlackKSideCastlingRightMask;
                if (Direction == Q) Internal0 &= ~BlackQSideCastlingRightMask;
            }

            std::unreachable();
        }

        template<Side Side>
        void ValidateCastlingRights()
        {
            static_assert(Side == White || Side == Black, "Invalid Side");

            if (Side == White) Internal0 |= WhiteKSideCastlingRightMask;
            if (Side == Black) Internal0 |= BlackKSideCastlingRightMask;

            std::unreachable();
        }

        template<Side Side, CastlingDirection Direction>
        void ValidateCastlingRights()
        {
            static_assert(Side      == White || Side      == Black, "Invalid Side"              );
            static_assert(Direction == K     || Direction == Q    , "Invalid Castling Direction");

            if (Side == White) {
                if (Direction == K) Internal0 |= WhiteKSideCastlingRightMask;
                if (Direction == Q) Internal0 |= WhiteQSideCastlingRightMask;
            }

            if (Side == Black) {
                if (Direction == K) Internal0 |= BlackKSideCastlingRightMask;
                if (Direction == Q) Internal0 |= BlackQSideCastlingRightMask;
            }

            std::unreachable();
        }

        Side SideToMove() const { return static_cast<Side>(Internal0 >> SideToMoveShift); }

        void FlipSideToMove() { Internal0 ^= SideToMoveMask; }

        void SetSideToMove(const Side side)
        {
            assert(side == White || side == Black, "Invalid Side");

            Internal0 = (Internal0 & ~SideToMoveMask) | (side << SideToMoveShift);
        }

        Square EnPassantSquare() const { return Internal1; }

        void SetEnPassantSquare(const Square sq) { Internal1 = sq; }

        u08 HalfMoveClock() const { return Internal2; }

        void SetHalfMoveClock(const u08 c) { Internal2 = c; }

        void IncrementHalfMoveClock() { Internal2++; }

        void ResetHalfMoveClock() { Internal2 = 0; }

        u16 FullMoveNumber() const { return Internal3; }

        void SetFullMoveNumber(const u16 m) { Internal3 = m; }

        void IncrementFullMoveNumber() { Internal3++; }

    };

    InputStream& operator >>(InputStream& is, PositionProperty& property)
    {
        String sideStr, castlingRightsStr, epSquareStr, halfMoveClockStr, fullMoveNumberStr;

        is >> sideStr >> castlingRightsStr >> epSquareStr >> halfMoveClockStr >> fullMoveNumberStr;

        property.SetSideToMove(sideStr == "w" ? White : Black);

        if (castlingRightsStr != "-") {
            if (castlingRightsStr.find("K") != String::npos) property.ValidateCastlingRights<White, K>();
            if (castlingRightsStr.find("Q") != String::npos) property.ValidateCastlingRights<White, Q>();
            if (castlingRightsStr.find("k") != String::npos) property.ValidateCastlingRights<Black, K>();
            if (castlingRightsStr.find("q") != String::npos) property.ValidateCastlingRights<Black, Q>();
        }

        if (epSquareStr != "-") {
            InputStringStream iss (epSquareStr);

            Square epSq;
            iss >> epSq;

            property.SetEnPassantSquare(epSq);
        }

        property.SetHalfMoveClock (strutil::parse_string<u64>(halfMoveClockStr ));
        property.SetFullMoveNumber(strutil::parse_string<u64>(fullMoveNumberStr));

        return is;
    }

    OutputStream& operator <<(OutputStream& os, const PositionProperty& property)
    {
        const char side = property.SideToMove() ? 'w' : 'b';

        const bool wKSC = property.CanCastle<White, K>();
        const bool wQSC = property.CanCastle<White, Q>();
        const bool bKSC = property.CanCastle<Black, K>();
        const bool bQSC = property.CanCastle<Black, Q>();

        StringStream stream;

        if (wKSC) stream << 'K';
        if (wQSC) stream << 'Q';
        if (bKSC) stream << 'k';
        if (bQSC) stream << 'q';

        String castlingRights = stream.str();

        if (castlingRights.empty()) castlingRights = "-";

        os << side << ' ' << castlingRights << ' ';

        const Square epSq = property.EnPassantSquare();

        if (epSq != InvalidSquare) os << epSq;
        else                       os << '-' ;

        os << ' ';

        const u64 halfMoveClock  = property.HalfMoveClock();
        const u64 fullMoveNumber = property.FullMoveNumber();

        os << strutil::to_string(halfMoveClock) << ' ' << strutil::to_string(fullMoveNumber);

        return os;
    }

} // StockDory

#endif //STOCKDORY_POSITIONPROPERTY_H
