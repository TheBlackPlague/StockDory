//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_POSITIONPROPERTY_H
#define STOCKDORY_POSITIONPROPERTY_H

#include "Base.h"
#include "Side.h"

namespace StockDory
{

    class PositionProperty
    {

        constexpr static u08 CastlingRightMask = 0b1111;

        constexpr static u08 WhiteKSideCastlingRightMask = 0b1000;
        constexpr static u08 WhiteQSideCastlingRightMask = 0b0100;
        constexpr static u08 BlackKSideCastlingRightMask = 0b0010;
        constexpr static u08 BlackQSideCastlingRightMask = 0b0001;

        constexpr static u08 SideToMoveMask  = 0b00010000;
        constexpr static u08 SideToMoveShift = 4;

        u08 Internal = White << SideToMoveShift | WhiteKSideCastlingRightMask | WhiteQSideCastlingRightMask
                                                | BlackKSideCastlingRightMask | BlackQSideCastlingRightMask;

        public:
        template<Side Side>
        bool CanCastleKSide() const
        {
            static_assert(Side == White || Side == Black, "Invalid Side");

            if (Side == White) return Internal & WhiteKSideCastlingRightMask;
            if (Side == Black) return Internal & BlackKSideCastlingRightMask;

            std::unreachable();
        }

        template<Side Side>
        bool CanCastleQSide() const
        {
            static_assert(Side == White || Side == Black, "Invalid Side");

            if (Side == White) return Internal & WhiteQSideCastlingRightMask;
            if (Side == Black) return Internal & BlackQSideCastlingRightMask;

            std::unreachable();
        }

        template<Side Side>
        void InvalidateCastlingRights()
        {
            static_assert(Side == White || Side == Black, "Invalid Side");

            if (Side == White) Internal &= ~(WhiteKSideCastlingRightMask | WhiteQSideCastlingRightMask);
            if (Side == Black) Internal &= ~(BlackKSideCastlingRightMask | BlackQSideCastlingRightMask);

            std::unreachable();
        }

        Side SideToMove() const { return static_cast<Side>(Internal >> SideToMoveShift); }

        void FlipSideToMove() { Internal ^= SideToMoveMask; }

    };

    OutputStream& operator <<(OutputStream& os, const PositionProperty& property)
    {
        const char side = property.SideToMove() ? 'w' : 'b';

        const bool wKSC = property.CanCastleKSide<White>();
        const bool wQSC = property.CanCastleQSide<White>();
        const bool bKSC = property.CanCastleKSide<Black>();
        const bool bQSC = property.CanCastleQSide<Black>();

        StringStream stream;

        if (wKSC) stream << 'K';
        if (wQSC) stream << 'Q';
        if (bKSC) stream << 'k';
        if (bQSC) stream << 'q';

        String castlingRights = stream.str();

        if (castlingRights.empty()) castlingRights = "-";

        os << side << ' ' << castlingRights;

        return os;
    }

} // StockDory

#endif //STOCKDORY_POSITIONPROPERTY_H
