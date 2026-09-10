//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "Backend/Type/Move.h"

struct FlagExpectation
{

    MoveFlag Flag;
    Piece Promotion;
    bool Capture;
    bool Castling;
    bool DoublePush;
    bool EnPassant;

};

constexpr std::array<FlagExpectation, 14> Expectations {{
    {MoveFlag::Quiet,                  NAP,    false, false, false, false},
    {MoveFlag::DoublePush,             NAP,    false, false, true,  false},
    {MoveFlag::KingCastle,             NAP,    false, true,  false, false},
    {MoveFlag::QueenCastle,            NAP,    false, true,  false, false},
    {MoveFlag::Capture,                NAP,    true,  false, false, false},
    {MoveFlag::EnPassant,              NAP,    true,  false, false, true },
    {MoveFlag::KnightPromotion,        Knight, false, false, false, false},
    {MoveFlag::BishopPromotion,        Bishop, false, false, false, false},
    {MoveFlag::RookPromotion,          Rook,   false, false, false, false},
    {MoveFlag::QueenPromotion,         Queen,  false, false, false, false},
    {MoveFlag::KnightCapturePromotion, Knight, true,  false, false, false},
    {MoveFlag::BishopCapturePromotion, Bishop, true,  false, false, false},
    {MoveFlag::RookCapturePromotion,   Rook,   true,  false, false, false},
    {MoveFlag::QueenCapturePromotion,  Queen,  true,  false, false, false}
}};

void Require(const bool condition, const std::string_view message)
{
    if (!condition) throw std::runtime_error(std::string(message));
}

int main()
{
    static_assert(sizeof(Move) == 2);
    static_assert(std::is_trivially_copyable_v<Move>);

    const Move empty;
    Require(!empty && empty.Promotion() == NAP && empty.ToString() == "0000", "Invalid null move");

    size_t checked = 0;

    for (Square from = A1; from != NASQ; from = Next(from))
        for (Square to = A1; to != NASQ; to = Next(to)) {
            if (from == to) continue;

            for (const auto& expected : Expectations) {
                const Move move (from, to, expected.Flag);

                Require(move && move.From() == from && move.To() == to, "Square encoding failed");
                Require(move.Flags() == expected.Flag, "Flag encoding failed");
                Require(move.Promotion() == expected.Promotion, "Promotion decoding failed");
                Require(move.Capture() == expected.Capture && move.Castling() == expected.Castling,
                        "Capture/castling classification failed");
                Require(move.DoublePush() == expected.DoublePush && move.EnPassant() == expected.EnPassant,
                        "Pawn move classification failed");
                Require(move.SameIdentity(Move::FromString(move.ToString())), "UCI identity round trip failed");
                Require(move.SameIdentity(Move(from, to, expected.Promotion)), "Identity excludes inferred flags");
                Require(move == Move(from, to, expected.Flag), "Full equality failed");

                for (const auto& other : Expectations) {
                    const Move second (from, to, other.Flag);
                    Require((move == second) == (expected.Flag == other.Flag), "Full equality ignores flags");
                    Require(move.SameIdentity(second) == (expected.Promotion == other.Promotion),
                            "Promotion changes move identity");
                }

                checked++;
            }
        }

    for (const auto invalid : {"", "0000", "a1a1", "a0a8", "a1j1", "a7a8k", "a7a8qq", "e2e"})
        Require(!Move::FromString(invalid), "Invalid UCI string accepted");

    std::cout << checked << " move encodings and all flag identity combinations passed\n";
}
