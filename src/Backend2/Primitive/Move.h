//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MOVE_H
#define STOCKDORY_MOVE_H

#include "Base.h"
#include "PieceType.h"
#include "Square.h"

namespace StockDory
{

    enum MoveFlag : u08
    {

        Normal  = 0b0000,
        Capture = 0b0001,

        EnPassant = 0b0011,

        KSideCastling = 0b0010,
        QSideCastling = 0b0100,

        KnightPromotionCapture = 0b0101,
        KnightPromotion        = 0b0110,
        BishopPromotionCapture = 0b0111,
        BishopPromotion        = 0b1000,
          RookPromotionCapture = 0b1001,
          RookPromotion        = 0b1010,
         QueenPromotionCapture = 0b1011,
         QueenPromotion        = 0b1100,

    };

    enum MoveNotation : u08 { UCI, SAN };

    template<MoveNotation Notation = UCI>
    class Move
    {

        constexpr static u16 SquareMask = 0b111111;

        constexpr static u16 TargetShift =  6;
        constexpr static u16   FlagShift = 12;

        u16 Internal;

        public:
        constexpr Move(const Square origin, const Square target, const MoveFlag flag)
        : Internal(origin | target << TargetShift | flag << FlagShift) {}

        constexpr Square Origin() const { return static_cast<Square>( Internal                 & SquareMask); }
        constexpr Square Target() const { return static_cast<Square>((Internal >> TargetShift) & SquareMask); }

        constexpr MoveFlag Flag() const { return static_cast<MoveFlag>(Internal >> FlagShift); }

        constexpr bool IsCapture() const { return Flag() & Capture; }

        constexpr bool IsPromotion() const { return Flag() >= QueenPromotionCapture; }

        constexpr PieceType PromotionType() const
        {
            const auto type = static_cast<PieceType>((Flag() - 3) / 2);

            assert(type == Knight || type == Bishop || type == Rook || type == Queen, "Invalid Promotion Type");

            return type;
        }

    };

    static_assert(Move(E7, E8, KnightPromotion).PromotionType() == Knight, "Promotion Type is not Knight");
    static_assert(Move(A7, A8, BishopPromotion).PromotionType() == Bishop, "Promotion Type is not Bishop");
    static_assert(Move(B7, B8,   RookPromotion).PromotionType() ==   Rook, "Promotion Type is not Rook"  );
    static_assert(Move(H7, H8,  QueenPromotion).PromotionType() ==  Queen, "Promotion Type is not Queen" );

    OutputStream& operator <<(OutputStream& os, const Move<> move)
    {
        os << move.Origin() << move.Target();

        if (!move.IsPromotion()) return os;

        switch (move.PromotionType()) {
            case Knight: os << 'n'; break;
            case Bishop: os << 'b'; break;
            case   Rook: os << 'r'; break;
            case  Queen: os << 'q'; break;

            default: std::unreachable();
        }

        return os;
    }

} // StockDory

#endif //STOCKDORY_MOVE_H
