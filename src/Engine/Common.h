//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_COMMON_H
#define STOCKDORY_COMMON_H

#include <chrono>

#include "../Backend/Misc.h"
#include "../Backend/Type/Move.h"

namespace StockDory
{

    using Score = int32_t;

    constexpr Score Mate = 31000;
    constexpr Score Draw =     0;

    constexpr Score Infinity = Mate + 1;
    constexpr Score     None = Mate + 2;

    constexpr uint8_t MaxDepth = 128;
    constexpr uint8_t MaxMove  = 218;

    constexpr Score MateInMaxDepth = Mate - MaxDepth * 4;

    constexpr uint16_t HistoryLimit = 16384;

    constexpr size_t MB = 1024 * 1024;

    constexpr size_t CacheLineSize = 64;

    using MS = std::chrono::milliseconds;
    using TP = std::chrono::time_point<std::chrono::steady_clock>;

    using KTable = Array<Move, 2, MaxDepth>;
    using HTable = Array<int16_t, 2, 6, 64>;

    bool IsMate(const Score score) { return abs(score) >= MateInMaxDepth; }

    bool IsWin (const Score score) { return score >=  MateInMaxDepth; }
    bool IsLoss(const Score score) { return score <= -MateInMaxDepth; }

    Score  WinIn(const uint8_t ply) { return  Mate - ply; }
    Score LossIn(const uint8_t ply) { return -Mate + ply; }

    uint8_t PlyToMate(const Score score)
    {
        return IsWin (score) ?  Mate - score :
               IsLoss(score) ? -Mate - score : 0;
    }

    static_assert(
        std::atomic<uint8_t >::is_always_lock_free &&
        std::atomic<uint16_t>::is_always_lock_free &&
        std::atomic<uint32_t>::is_always_lock_free &&
        std::atomic<uint64_t>::is_always_lock_free &&
        std::atomic< int8_t >::is_always_lock_free &&
        std::atomic< int16_t>::is_always_lock_free &&
        std::atomic< int32_t>::is_always_lock_free &&
        std::atomic< int64_t>::is_always_lock_free &&
        std::atomic<char    >::is_always_lock_free &&
        std::atomic<char8_t >::is_always_lock_free &&
        std::atomic<char16_t>::is_always_lock_free &&
        std::atomic<char32_t>::is_always_lock_free &&
        std::atomic<  bool  >::is_always_lock_free  ,
        "StockDory relies on lock-free atomics"
    );

    static_assert(
        std::atomic_ref<uint8_t >::is_always_lock_free &&
        std::atomic_ref<uint16_t>::is_always_lock_free &&
        std::atomic_ref<uint32_t>::is_always_lock_free &&
        std::atomic_ref<uint64_t>::is_always_lock_free &&
        std::atomic_ref< int8_t >::is_always_lock_free &&
        std::atomic_ref< int16_t>::is_always_lock_free &&
        std::atomic_ref< int32_t>::is_always_lock_free &&
        std::atomic_ref< int64_t>::is_always_lock_free &&
        std::atomic_ref<char    >::is_always_lock_free &&
        std::atomic_ref<char8_t >::is_always_lock_free &&
        std::atomic_ref<char16_t>::is_always_lock_free &&
        std::atomic_ref<char32_t>::is_always_lock_free &&
        std::atomic_ref<  bool  >::is_always_lock_free  ,
        "StockDory relies on lock-free atomic references"
    );

} // StockDory

#endif //STOCKDORY_COMMON_H
