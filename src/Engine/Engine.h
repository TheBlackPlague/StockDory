//
// Copyright (c) 2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_ENGINE_H
#define STOCKDORY_ENGINE_H

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <expected>
#include <limits>
#include <optional>
#include <string_view>

#include "Perft.h"
#include "ThreadedSearch.h"

namespace StockDory
{

    struct SearchRequest
    {

        uint64_t Nodes = std::numeric_limits<uint64_t>::max();
        uint8_t  Depth = MaxDepth - 1;

        std::optional<MS> MoveTime {};

        Array<std::optional<MS>, 2> Time      {};
        Array<             MS  , 2> Increment {};

        uint16_t MovesToGo = 0;

    };

    namespace Engine
    {

        inline Board    Board      {};
        RepetitionStack Repetition {};

        uint8_t  HalfMoveClock  = 0;
        uint16_t FullMoveNumber = 1;

        std::optional<MS> MoveOverhead {};

        void Stop() { ThreadedSearch::Stop(); }
        void Wait() { ThreadedSearch::Wait(); }

        Limit SearchTimeAllocation(const SearchRequest& request, const Side side, const uint16_t fullMoveNumber)
        {
            Limit limit
            {
                .Nodes = request.Nodes,
                .Depth = std::clamp<uint8_t>(request.Depth, 1, MaxDepth - 1)
            };

            if (request.MoveTime) {
                limit.Timed = true;
                limit.Fixed = true;

                limit. ActualTime = *request.MoveTime;
                limit.   BaseTime = *request.MoveTime;
                limit.OptimalTime = *request.MoveTime;

                return limit;
            }

            if (!request.Time[side]) return limit;

            limit.Timed = true ;
            limit.Fixed = false;

            const uint64_t time = request.Time     [side]->count();
            const uint64_t inc  = request.Increment[side] .count();

            const uint64_t overhead = MoveOverhead.value_or(MS(TimeProcessingOverhead)).count();

            const uint64_t usableTime = time > overhead ? time - overhead : 0;

            const uint64_t incTime = inc / TimeIncrementPartitionDenominator * TimeIncrementPartitionNumerator +
                                     inc % TimeIncrementPartitionDenominator * TimeIncrementPartitionNumerator /
                                           TimeIncrementPartitionDenominator                                   ;

            const uint64_t clkTime = usableTime / TimeHardLimitClockDenominator * TimeHardLimitClockNumerator +
                                     usableTime % TimeHardLimitClockDenominator * TimeHardLimitClockNumerator /
                                                  TimeHardLimitClockDenominator                               ;

            uint64_t baseTimeForMove;

            if (request.MovesToGo > 0) {
                baseTimeForMove = usableTime / request.MovesToGo + (usableTime % request.MovesToGo != 0);
            } else {
                const double decayFactor = std::exp(-TimeSoftFractionRate / 1000.0 * fullMoveNumber);

                const double fraction = (TimeSoftFractionMax - TimeSoftFractionDecay * decayFactor) / 1000.0;

                baseTimeForMove = static_cast<uint64_t>(static_cast<double>(usableTime) * std::max(fraction, 0.0));
            }

            const uint64_t allocatedTime = baseTimeForMove + std::min<uint64_t>(incTime, usableTime - baseTimeForMove);

            const uint64_t  actualTime = std::min<uint64_t>(allocatedTime * TimeHardLimitMultiplier,    clkTime);
            const uint64_t optimalTime = std::min<uint64_t>(allocatedTime                          , actualTime);

            limit. ActualTime = MS( actualTime);
            limit.   BaseTime = MS(optimalTime);
            limit.OptimalTime = MS(optimalTime);

            return limit;
        }

        void Start(const SearchRequest& request)
        {
            const Limit limit = SearchTimeAllocation(request, Board.SideToMove(), FullMoveNumber);

            ThreadedSearch::Start(limit, Board, Repetition, HalfMoveClock);
        }

        SearchResult Search(const SearchRequest& request)
        {
            Start(request);
            Wait ();

            return ThreadedSearch::Result;
        }

        void NewGame()
        {
            Stop();
            Wait();

            const size_t parts = ThreadPool.Size();

            ThreadPool.For(parts, [parts](const size_t part) -> void { TT.Clear(part, parts); });

            Board      = {};
            Repetition = {};

            Repetition.Push(Board.Zobrist());

            HalfMoveClock  = 0;
            FullMoveNumber = 1;
        }

        void SetThreads(const size_t threads) { ThreadedSearch::Resize(threads); }

        uint16_t HashFull() { return TT.HashFull(); }

        void Shutdown()
        {
            Stop();
            Wait();

            ThreadedSearch::Release();
        }

        std::expected<void, std::string_view> SetPosition(const std::string_view fen)
        {
            const auto board = StockDory::Board::FromFen(fen);

            if (!board) return std::unexpected(board.error());

            Array<uint64_t, 2> counters { 0, 1 };

            size_t offset = 0;

            for (size_t field = 0; ; field++) {
                offset = fen.find_first_not_of(' ', offset);

                if (offset == std::string_view::npos) break;

                const size_t end = std::min(fen.find(' ', offset), fen.size());

                const std::string_view text = fen.substr(offset, end - offset);

                offset = end;

                if (field < 4) continue;
                if (field > 5) return std::unexpected("Invalid FEN: more than six fields");

                const auto [last, error] = std::from_chars(text.data(), text.data() + text.size(), counters[field - 4]);

                if (error != std::errc() || last != text.data() + text.size())
                    return std::unexpected("Invalid FEN move counters");
            }

            Board      = *board;
            Repetition = {};

            Repetition.Push(Board.Zobrist());

            HalfMoveClock  = static_cast<uint8_t >(std::min<uint64_t>(counters[0], 254  ));
            FullMoveNumber = static_cast<uint16_t>(std::min<uint64_t>(counters[1], 65535));

            return {};
        }

        std::optional<Move> ParseMove(const std::string_view text)
        {
            if (text.size() != 4 && text.size() != 5) return {};

            const Square from = SquareFromString(text.substr(0, 2));
            const Square to   = SquareFromString(text.substr(2, 2));

            if (from == NASQ || to == NASQ || from == to) return {};

            PieceType promotion = NAPT;

            if (text.size() == 5) {
                const size_t letter = std::string_view("nbrq").find(text[4]);

                if (letter == std::string_view::npos) return {};

                promotion = static_cast<PieceType>(Knight + letter);
            }

            const Move move = Board.ConstructMove(from, to, promotion);

            return WithSide(
                Board.SideToMove(),
                [move]<Side Us> -> std::optional<Move>
                {
                    for (const Move legal : MoveList<Us>(Board)) if (legal == move) return move;

                    return {};
                }
            );
        }

        void Play(const Move move)
        {
            const bool irreversible = move.Capture() || Board[move.From()].Type() == Pawn;

            HalfMoveClock = irreversible ? 0 : std::min<uint8_t>(HalfMoveClock + 1, 254);

            if (Board.SideToMove() == Black) FullMoveNumber++;

            Board.MakeMove<Hashed>(move);

            if (irreversible || HalfMoveClock == 254) Repetition = {};

            Repetition.Push(Board.Zobrist());
        }

        template<typename OnMove>
        PerftResult Perft(const uint8_t depth, PerftMode mode, const OnMove& onMove)
        {
            Stop();
            Wait();

            if (depth == 0) return { .Nodes = 1 };

            mode.Parallel = ThreadPool.Size() > 1;

            PerftResult result;

            const TP start = std::chrono::steady_clock::now();

            ThreadPool.Run(
                [&](const size_t index) -> void
                {
                    if (index != 0) return;

                    result.Nodes = WithSide(
                        Board.SideToMove(),
                        [&]<Side Us> -> uint64_t { return PerftRoot<Us>(Board, depth, mode, onMove); }
                    );
                }
            );

            result.Time = std::chrono::steady_clock::now() - start;

            return result;
        }

        template<typename Handler>
        void Subscribe(Handler& handler)
        {
            Stop();
            Wait();

            EventHandler = SearchEventSink(handler);
        }

        void SetHash(const size_t bytes)
        {
            Stop();
            Wait();

            TT.Resize(bytes);
        }

        void SetMoveOverhead(const MS overhead)
        {
            Stop();
            Wait();

            MoveOverhead = overhead;
        }

        void ClearHash()
        {
            Stop();
            Wait();

            const size_t parts = ThreadPool.Size();

            ThreadPool.For(parts, [parts](const size_t part) -> void { TT.Clear(part, parts); });
        }

        bool Searching() { return ThreadedSearch::Searching(); }

    } // Engine

} // StockDory

#endif //STOCKDORY_ENGINE_H
