//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_COMMON_H
#define STOCKDORY_COMMON_H

#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace StockDory
{

    template<typename T, typename... Args>
    T& ReplaceInline(T& object, Args&&... args)
    {
        T* address = std::addressof(object);

        std::destroy_at(address);

        return *std::construct_at(address, std::forward<Args>(args)...);
    }

    template<typename T, size_t N, size_t... Ns>
    struct IArray { using Internal = std::array<typename IArray<T, Ns...>::Internal, N>; };

    template<typename T, size_t N>
    struct IArray<T, N> { using Internal = std::array<T, N>; };

    template<typename T, size_t... Ns>
    using Array = IArray<T, Ns...>::Internal; // Fixed size N-dimensional array of values

    template<typename T, size_t N, typename V>
    constexpr void Fill(std::array<T, N>& array, const V& value)
    {
        if constexpr (requires { array.fill(value); }) array.fill(value);
        else for (auto& sub : array) Fill(sub, value);
    }

    enum Side : uint8_t
    {

        White,
        Black,
        NAS

    };

    constexpr Side Opposite(const Side side)
    {
        return static_cast<Side>(static_cast<uint8_t>(side) ^ 0x1);
    }

    template<typename F>
    [[clang::always_inline]]
    constexpr decltype(auto) WithSide(const Side side, F&& f)
    {
        return side == White ? f.template operator ()<White>() : f.template operator ()<Black>();
    }

    enum PieceType : uint8_t
    {

        Pawn,
        Knight,
        Bishop,
        Rook,
        Queen,
        King,
        NAPT

    };

    constexpr PieceType Next(const PieceType type)
    {
        return static_cast<PieceType>(static_cast<uint8_t>(type) + 1);
    }

    constexpr char FirstLetter(const PieceType type)
    {
        // ReSharper disable once StringLiteralTypo
        return std::string_view("PNBRQK ")[type];
    }

    enum Square : uint8_t
    {

        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8, NASQ

    };

    constexpr Square Next(const Square sq)
    {
        return static_cast<Square>(static_cast<uint8_t>(sq) + 1);
    }

    constexpr uint8_t FileOf(const Square sq) { return sq % 8; }
    constexpr uint8_t RankOf(const Square sq) { return sq / 8; }

    constexpr Square MakeSquare(const uint8_t file, const uint8_t rank)
    {
        return static_cast<Square>(rank * 8 + file);
    }

    std::string ToString(const Square sq)
    {
        return { static_cast<char>('a' + FileOf(sq)), static_cast<char>('1' + RankOf(sq)) };
    }

    constexpr Square SquareFromString(const std::string_view text)
    {
        if (text.size() != 2) return NASQ;

        const char file = text[0] >= 'A' && text[0] <= 'H' ? static_cast<char>(text[0] + ('a' - 'A')) : text[0];
        const char rank = text[1];

        if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return NASQ;

        return MakeSquare(file - 'a', rank - '1');
    }

    enum Direction : int8_t
    {

        North     =  8,
        NorthEast =  9,
        SouthEast = -7,
             East =  1,

        South     = -8,
        SouthWest = -9,
        NorthWest =  7,
             West = -1,

    };

    constexpr Square operator +(const Square sq, const Direction direction)
    {
        return static_cast<Square>(static_cast<uint8_t>(sq) + direction);
    }

    constexpr Square operator -(const Square sq, const Direction direction)
    {
        return static_cast<Square>(static_cast<uint8_t>(sq) - direction);
    }

    template<Side S>
    constexpr Direction Up = S == White ? North : South;

    template<Side S>
    constexpr Direction UpEast = S == White ? NorthEast : SouthEast;

    template<Side S>
    constexpr Direction UpWest = S == White ? NorthWest : SouthWest;

    template<Side S>
    constexpr Direction Down = S == White ? South : North;

    template<Side S>
    constexpr Direction DownEast = S == White ? SouthEast : NorthEast;

    template<Side S>
    constexpr Direction DownWest = S == White ? SouthWest : NorthWest;

    template<Side S>
    constexpr uint8_t RelativeRank(const uint8_t rank) { return S == White ? rank : static_cast<uint8_t>(7 - rank); }

    struct Piece
    {

        private:
        constexpr static uint8_t TypeMask = 0xF;
        constexpr static uint8_t SidePos  =   4;

        // [    SIDE     ] [    TYPE    ]
        // [   4 BITS    ] [   4 BITS   ]
        uint8_t Internal;

        public:
        constexpr Piece()
        {
            Internal = NAPT | NAS << 4;
        }

        constexpr Piece(const PieceType type, const Side side)
        {
            Internal = type | side << 4;
        }

        [[nodiscard]]
        constexpr PieceType Type() const
        {
            return static_cast<PieceType>(Internal & TypeMask);
        }

        [[nodiscard]]
        constexpr Side Side() const
        {
            return static_cast<enum Side>(Internal >> SidePos);
        }

        [[nodiscard]]
        constexpr bool operator ==(const Piece&) const = default;

    };

    using BitBoard = uint64_t;

    constexpr BitBoard BBDefault = 0x0000000000000000ULL;
    constexpr BitBoard BBFilled  = 0xFFFFFFFFFFFFFFFFULL;

    constexpr uint8_t Count(const BitBoard bb)
    {
        return std::popcount(bb);
    }

    constexpr bool Get(const BitBoard bb, const Square sq)
    {
        assert(sq < NASQ);

        return bb >> sq & 1ULL;
    }

    constexpr BitBoard FromSquare(const Square sq)
    {
        assert(sq < NASQ);

        return 1ULL << sq;
    }

    constexpr Square ToSquare(const BitBoard bb)
    {
        return static_cast<Square>(std::countr_zero(bb));
    }

    constexpr void Set  (BitBoard& bb, const Square sq) { bb |=  FromSquare(sq); }
    constexpr void Clear(BitBoard& bb, const Square sq) { bb &= ~FromSquare(sq); }

    class Squares
    {

        BitBoard Internal;

        public:
        struct Iterator
        {

            BitBoard Remaining;

            constexpr Square operator *() const { return ToSquare(Remaining); }

            constexpr Iterator& operator ++() { Remaining &= Remaining - 1; return *this; }

            constexpr bool operator !=(const std::default_sentinel_t) const { return Remaining != BBDefault; }

        };

        constexpr explicit Squares(const BitBoard bb) : Internal(bb) {}

        constexpr Iterator begin() const { return { Internal }; }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        constexpr std::default_sentinel_t end() const { return {}; }

    };

    template<Direction Direction>
    constexpr BitBoard Shift(const BitBoard bb)
    {
        constexpr BitBoard NotFileA = 0xFEFEFEFEFEFEFEFEULL;
        constexpr BitBoard NotFileH = 0x7F7F7F7F7F7F7F7FULL;

        const BitBoard source = Direction == East || Direction == NorthEast || Direction == SouthEast ? bb & NotFileH :
                                Direction == West || Direction == NorthWest || Direction == SouthWest ? bb & NotFileA :
                                                                                                        bb            ;

        return Direction > 0 ? source << Direction : source >> -Direction;
    }

    struct PinBitBoard
    {

        BitBoard Straight = BBDefault;
        BitBoard Diagonal = BBDefault;

    };

    struct CheckBitBoard
    {

        BitBoard Check       = BBDefault;
        bool     DoubleCheck = false;

    };

    enum class MoveFlag : uint8_t
    {

        Base                   =  0,
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
        uint16_t Internal;

        public:
        constexpr Move() = default;

        constexpr Move(const Square from, const Square to, const PieceType promotion = NAPT) noexcept
        : Move(from, to, promotion == NAPT ? MoveFlag::Base : static_cast<MoveFlag>(8 + promotion - Knight))
        {
            assert(promotion == NAPT || (promotion >= Knight && promotion <= Queen));
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
        constexpr PieceType Promotion() const noexcept
        {
            return Internal & PromotionMask ? static_cast<PieceType>((Internal >> FlagPos & 3) + Knight) : NAPT;
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
        constexpr bool Tactical() const noexcept
        {
            return Internal & CaptureMask || Internal & PromotionMask;
        }

        [[nodiscard]]
        constexpr bool Quiet() const noexcept
        {
            return !Tactical();
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

        constexpr explicit operator bool() const noexcept { return Internal != 0; }

    };

    static_assert(sizeof(Move) == sizeof(uint16_t));
    static_assert(std::is_trivially_copyable_v<Move>);

    enum MakeMode : uint8_t
    {

        Plain,
        Hashed

    };

    enum MoveSet : uint8_t
    {

          CaptureMoves = 0b001,
        PromotionMoves = 0b010,
            QuietMoves = 0b100,

        NoisyMoves = CaptureMoves | PromotionMoves,
          AllMoves =   NoisyMoves |     QuietMoves

    };

    enum Ordering : uint8_t
    {

        Unordered,
        Ordered

    };

    enum CastlingSide : uint8_t
    {

         KingSide,
        QueenSide

    };

    using ZobristHash = uint64_t;

    constexpr uint8_t MaxDepth = 246;
    constexpr uint8_t MaxMove  = 218;

    constexpr size_t MB = 1024 * 1024;

    using MS = std::chrono::milliseconds;
    using TP = std::chrono::time_point<std::chrono::steady_clock>;

    using Score = int32_t;

    constexpr Score Mate = 31000;
    constexpr Score Draw =     0;

    constexpr Score Infinity = Mate + 1;
    constexpr Score     None = Mate + 2;

    constexpr Score MateInMaxDepth = Mate - MaxDepth * 4;

    bool IsMate(const Score score) { return abs(score) >= MateInMaxDepth; }

    bool IsWin (const Score score) { return score >=  MateInMaxDepth; }
    bool IsLoss(const Score score) { return score <= -MateInMaxDepth; }

    Score  WinIn(const uint8_t ply) { return  Mate - ply; }
    Score LossIn(const uint8_t ply) { return -Mate + ply; }

    Score PlyToMate(const Score score)
    {
        return IsWin (score) ?  Mate - score :
               IsLoss(score) ? -Mate - score : 0;
    }

    constexpr uint16_t AspirationWindowWideningQuantization =   128;
    constexpr uint16_t                      LMRQuantization =  1024;
    constexpr uint16_t          MaterialScalingQuantization = 16384;

    constexpr uint16_t HistoryLimit = 16384;

    constexpr size_t  CorrectionHistorySize         = 16384;
    constexpr int32_t CorrectionHistoryLimit        =  1024;
    constexpr int32_t CorrectionHistoryQuantization =  1024;

    using KTable = Array<Move, 2, MaxDepth>;

    using        HTable = Array<int16_t, 2, 6, 64   >;
    using CaptureHTable = Array<int16_t, 2, 6, 64, 6>;

    using  CHTable = Array<HTable , 2, 6, 64       >;
    using CCHTable = Array<int16_t, 2, 6, 64, 6, 64>;

    using MinorCTable = Array<int16_t    , 2, CorrectionHistorySize>;
    using MajorCTable = Array<MinorCTable, 2                       >;

    constexpr HTable NullHistory = {};

    using MemoryOrder = std::memory_order;

    constexpr size_t DestructiveAlignment = std::hardware_destructive_interference_size;

    template<typename T>
    class Atomic
    {

        static_assert(std::is_trivially_copyable_v<T>, "Atomic values must be trivially copyable");

        static_assert(std::atomic<T>::is_always_lock_free, "Atomic values must be lock-free");

        std::atomic<T> Internal {};

        public:
        constexpr Atomic() noexcept = default;

        constexpr explicit Atomic(const T value) noexcept : Internal(value) {}

        Atomic(const Atomic&  other) noexcept : Internal(other.Load(MemoryOrder::relaxed)) {}
        Atomic(      Atomic&& other) noexcept : Internal(other.Load(MemoryOrder::relaxed)) {}

        Atomic& operator =(const Atomic& other) noexcept
        {
            Store(other.Load(MemoryOrder::relaxed), MemoryOrder::relaxed);

            return *this;
        }

        Atomic& operator =(Atomic&& other) noexcept
        {
            Store(other.Load(MemoryOrder::relaxed), MemoryOrder::relaxed);

            return *this;
        }

        [[nodiscard]]
        T Load(const MemoryOrder order = MemoryOrder::seq_cst) const noexcept
        {
            return Internal.load(order);
        }

        void Store(const T value, const MemoryOrder order = MemoryOrder::seq_cst) noexcept
        {
            Internal.store(value, order);
        }

        [[nodiscard]]
        T Exchange(const T value, const MemoryOrder order = MemoryOrder::seq_cst) noexcept
        {
            return Internal.exchange(value, order);
        }

        T Add(const T value, const MemoryOrder order = MemoryOrder::seq_cst) noexcept
        {
            return Internal.fetch_add(value, order) + value;
        }

        T Subtract(const T value, const MemoryOrder order = MemoryOrder::seq_cst) noexcept
        {
            return Internal.fetch_sub(value, order) - value;
        }

        void Wait(const T expected, const MemoryOrder order = MemoryOrder::seq_cst) const noexcept
        {
            Internal.wait(expected, order);
        }

        void NotifyOne() noexcept { Internal.notify_one(); }
        void NotifyAll() noexcept { Internal.notify_all(); }

    };

    template<typename T>
    struct alignas(DestructiveAlignment) CacheLine : T { using T::T; };

    [[noreturn]]
    void UnrecoverableError()
    {
        std::cerr << "An unrecoverable error has occurred." << std::endl;
        std::abort();
    }

} // StockDory

#endif //STOCKDORY_COMMON_H
