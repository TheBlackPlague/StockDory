//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_POSITIONINFO_H
#define STOCKDORY_POSITIONINFO_H

#include "Base.h"
#include "Side.h"
#include "Zobrist.h"

namespace StockDory
{

    enum CastlingFlag : u08
    {

        KWhite = 0b0001,
        QWhite = 0b0010,
        KBlack = 0b0100,
        QBlack = 0b1000,

    };

    struct PositionInfo
    {

        constexpr static u08 CastlingMask = 0b1111;

        constexpr static u08 SideToMoveShift = 4;
        constexpr static u08 SideToMoveMask  = 0b1 << SideToMoveShift;

        u08 CastlingSideToMove = SideToMoveMask | CastlingMask;

        Square EnPassant = InvalidSquare;

        u08 HalfMoveClock  = 0;
        u16 FullMoveNumber = 1;

        Side SideToMove() const { return static_cast<Side>(CastlingSideToMove >> SideToMoveShift); }

        template<Side Side>
        void SideToMove() { CastlingSideToMove |= (Side << SideToMoveShift); }

        void FlipSideToMove() { CastlingSideToMove ^= SideToMoveMask; }

        template<CastlingFlag Flag>
        bool Castling() const { return CastlingSideToMove & Flag; }

        template<CastlingFlag Flag, bool Enable>
        void Castling()
        {
            if (Enable) CastlingSideToMove |=  Flag;
            else        CastlingSideToMove &= ~Flag;
        }

        u08 CastlingRaw() const { return CastlingSideToMove | CastlingMask; }

    };

    InputStream& operator >>(InputStream& is, PositionInfo& info)
    {
        String sideStr, castlingRightsStr, epSquareStr, halfMoveClockStr, fullMoveNumberStr;

        is >> sideStr >> castlingRightsStr >> epSquareStr >> halfMoveClockStr >> fullMoveNumberStr;

        (sideStr == "w" ? info.SideToMove<White>() : info.SideToMove<Black>());

        if (castlingRightsStr != "-") {
            (castlingRightsStr.find("K") != String::npos ? info.Castling<KWhite, true >() :
                                                               info.Castling<KWhite, false>());
            (castlingRightsStr.find("Q") != String::npos ? info.Castling<QWhite, true >() :
                                                               info.Castling<QWhite, false>());
            (castlingRightsStr.find("k") != String::npos ? info.Castling<KBlack, true >() :
                                                               info.Castling<KBlack, false>());
            (castlingRightsStr.find("q") != String::npos ? info.Castling<QBlack, true >() :
                                                               info.Castling<QBlack, false>());
        }

        if (epSquareStr != "-") {
            InputStringStream iss (epSquareStr);

            iss >> info.EnPassant;
        }

        info.HalfMoveClock  = strutil::parse_string<u64>(halfMoveClockStr );
        info.FullMoveNumber = strutil::parse_string<u64>(fullMoveNumberStr);

        return is;
    }

    OutputStream& operator <<(OutputStream& os, const PositionInfo& info)
    {
        const char side = info.SideToMove() ? 'w' : 'b';

        const bool wKSC = info.Castling<KWhite>();
        const bool wQSC = info.Castling<QWhite>();
        const bool bKSC = info.Castling<KBlack>();
        const bool bQSC = info.Castling<QBlack>();

        StringStream stream;

        if (wKSC) stream << 'K';
        if (wQSC) stream << 'Q';
        if (bKSC) stream << 'k';
        if (bQSC) stream << 'q';

        String castlingRights = stream.str();

        if (castlingRights.empty()) castlingRights = "-";

        os << side << ' ' << castlingRights << ' ';

        if (info.EnPassant != InvalidSquare) os << info.EnPassant;
        else                                 os << '-';

        os << ' ';

        os << static_cast<u64>(info.HalfMoveClock) << ' ' << static_cast<u32>(info.FullMoveNumber);

        return os;
    }

} // StockDory

#endif //STOCKDORY_POSITIONINFO_H
