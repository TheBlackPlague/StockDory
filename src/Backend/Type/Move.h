//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_MOVE_H
#define STOCKDORY_MOVE_H

#include <cassert>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "Piece.h"
#include "Square.h"

enum class MoveFlag : uint8_t
{

    Quiet                  =  0,
    DoublePush             =  1,
    KingCastle             =  2,
    QueenCastle            =  3,
    Capture                =  4,
    EnPassant              =  5,
    KnightPromotion        =  8,
    BishopPromotion        =  9,
    RookPromotion          = 10,
    QueenPromotion         = 11,
    KnightCapturePromotion = 12,
    BishopCapturePromotion = 13,
    RookCapturePromotion   = 14,
    QueenCapturePromotion  = 15

};

struct Move
{

    private:
    constexpr static uint16_t SquareMask    = 0x003F;
    constexpr static uint16_t CaptureMask   = 0x4000;
    constexpr static uint16_t PromotionMask = 0x8000;
    constexpr static uint8_t  ToPos         =      6;
    constexpr static uint8_t  FlagPos       =     12;

    // [      FLAGS      ] [     TO     ] [    FROM    ]
    // [     4 BITS      ] [   6 BITS   ] [   6 BITS   ]
    uint16_t Internal = 0;

    public:
    [[nodiscard]]
    static constexpr Move FromString(const std::string_view str)
    {
        if (str.size() != 4 && str.size() != 5) return {};

        const Square from = ::FromString(str.substr(0, 2));
        const Square to   = ::FromString(str.substr(2, 2));

        if (from == NASQ || to == NASQ || from == to) return {};

        Piece promotion = NAP;

        if (str.size() == 5) {
            switch (str[4]) {
                case 'q':
                    promotion = Queen;
                    break;
                case 'r':
                    promotion = Rook;
                    break;
                case 'b':
                    promotion = Bishop;
                    break;
                case 'n':
                    promotion = Knight;
                    break;
                default:
                    return {};
            }
        }

        return Move(from, to, promotion);
    }

    constexpr Move() = default;

    constexpr Move(const Square from, const Square to, const Piece promotion = NAP) noexcept
        : Move(from, to, promotion == NAP ? MoveFlag::Quiet : static_cast<MoveFlag>(8 + promotion - Knight))
    {
        assert(promotion == NAP || (promotion >= Knight && promotion <= Queen));
    }

    constexpr Move(const Square from, const Square to, const MoveFlag flag) noexcept
    {
        assert(from < NASQ && to < NASQ);

        Internal = from | to << ToPos | static_cast<uint16_t>(flag) << FlagPos;
    }

    [[nodiscard]]
    constexpr Square From() const noexcept
    {
        return static_cast<Square>(Internal & SquareMask);
    }

    [[nodiscard]]
    constexpr Square To() const noexcept
    {
        return static_cast<Square>(Internal >> ToPos & SquareMask);
    }

    [[nodiscard]]
    constexpr MoveFlag Flags() const noexcept
    {
        return static_cast<MoveFlag>(Internal >> FlagPos);
    }

    [[nodiscard]]
    constexpr bool Capture() const noexcept
    {
        return Internal & CaptureMask;
    }

    [[nodiscard]]
    constexpr Piece Promotion() const noexcept
    {
        return Internal & PromotionMask ? static_cast<Piece>((Internal >> FlagPos & 3) + Knight) : NAP;
    }

    [[nodiscard]]
    constexpr bool Castling() const noexcept
    {
        return (Internal & 0xE000) == 0x2000;
    }

    [[nodiscard]]
    constexpr bool DoublePush() const noexcept
    {
        return Flags() == MoveFlag::DoublePush;
    }

    [[nodiscard]]
    constexpr bool EnPassant() const noexcept
    {
        return Flags() == MoveFlag::EnPassant;
    }

    [[nodiscard]]
    constexpr bool SameIdentity(const Move other) const noexcept
    {
        return (Internal & 0x0FFF) == (other.Internal & 0x0FFF) && Promotion() == other.Promotion();
    }

    [[nodiscard]]
    constexpr bool operator ==(const Move other) const noexcept
    {
        return Internal == other.Internal;
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator bool() const noexcept { return Internal != 0; }

    [[nodiscard]]
    std::string ToString() const
    {
        if (!*this) return "0000";

        std::string result = ::ToString(From()) + ::ToString(To());

        if (const Piece promotion = Promotion(); promotion != NAP)
            result += static_cast<char>(FirstLetter(promotion) + ('a' - 'A'));

        return result;
    }

};

static_assert(sizeof(Move) == sizeof(uint16_t));
static_assert(std::is_trivially_copyable_v<Move>);

#endif //STOCKDORY_MOVE_H
