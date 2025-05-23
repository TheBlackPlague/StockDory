//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEARCH2_H
#define STOCKDORY_SEARCH2_H

#include "../Backend/Board.h"
#include "../Backend/Misc.h"
#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Move.h"

#include "Common.h"
#include "TranspositionTable.h"
#include "TunableParameter.h"

namespace StockDory
{

    enum SearchTranspositionEntryType : uint8_t
    {

        Invalid,

        Exact,
        Beta ,
        Alpha

    };

    using CompressedHash  = uint16_t;
    using CompressedScore =  int16_t;

    constexpr CompressedScore CompressedInfinity = 32000;

    CompressedHash  CompressHash (const ZobristHash hash) { return hash; }
    CompressedScore CompressScore(const Score      score)
    {
        return std::clamp<Score>(score, -CompressedInfinity, CompressedInfinity);
    }

    struct SearchTranspositionEntry
    {

        using EntryType = SearchTranspositionEntryType;

        CompressedHash  Hash       = 0;
        CompressedScore Evaluation = 0;
        Move            Move       = ::Move();
        uint8_t         Depth      = 0;
        EntryType       Type       = Invalid;

    };

    inline TranspositionTable<SearchTranspositionEntry> TT (16 * MB);

    using KTable = Array<Move, 2, MaxDepth>;
    using HTable = Array<int16_t, 2, 6, 64>;

    class SearchStack
    {

        public:
        struct Frame
        {

            Score   StaticEvaluation = 0;
            uint8_t HalfMoveCounter  = 0;

        };

        constexpr static size_t Padding = 8;

        private:
        Array<Frame, Padding + MaxDepth> Internal {};

        public:
              Frame& operator[](const size_t index)       { return Internal[index - Padding]; }
        const Frame& operator[](const size_t index) const { return Internal[index - Padding]; }

    };

    constexpr size_t RepetitionLimit = 3;

    class RepetitionStack
    {

        Array<ZobristHash, 4096> Internal {};

        size_t CurrentIndex = 0;

        public:
        void Push(const ZobristHash hash) { Internal[CurrentIndex++] = hash; }

        void Pop() { CurrentIndex--; }

        bool Found(const ZobristHash hash, const uint8_t halfMoveCounter) const
        {
            size_t count = 0;
            for (size_t i = CurrentIndex - 1; i != std::numeric_limits<size_t>::max(); i--) {
                if (i < CurrentIndex - 1 - halfMoveCounter) return false;

                if (Internal[i] == hash) {
                    count++;
                    if (count > RepetitionLimit - 1) return true;
                }
            }

            return false;
        }

    };

    using PV = Array<Move, MaxDepth>;

    struct PVEntry
    {

        uint8_t Ply;
        PV      PV ;

    };

    using PVTable = Array<PVEntry, MaxDepth>;

    enum SearchThreadStatus : bool
    {

        Stopped,
        Running

    };

    enum SearchThreadType : uint8_t
    {

        Main,
        Parallel

    };

    struct DefaultSearchEventHandler
    {

        static void HandleDepthIteration(
            [[maybe_unused]] const uint8_t           depth,
            [[maybe_unused]] const uint8_t  selectiveDepth,
            [[maybe_unused]] const Score        evaluation,
            [[maybe_unused]] const uint64_t          nodes,
            [[maybe_unused]] const uint64_t        ttNodes,
            [[maybe_unused]] const MS                 time,
            [[maybe_unused]] const PVEntry&             pv) {}

        static void HandleBestMove([[maybe_unused]] const Move move) {}

    };

    struct Limit
    {

        uint64_t Nodes = std::numeric_limits<uint64_t>::max();
        uint8_t  Depth = MaxDepth / 2;

        bool Timed = false;

        TP Origin = std::chrono::steady_clock::now();

        MS ActualTime  {};
        MS OptimalTime {};

        void Start() { Origin = std::chrono::steady_clock::now(); }

        MS Elapsed() const
        {
            return std::chrono::duration_cast<MS>(std::chrono::steady_clock::now() - Origin);
        }

        bool Crossed(const uint64_t nodes, const uint8_t depth) const
        {
            if (!Timed) return nodes > Nodes || depth > Depth;

            return Elapsed() > OptimalTime || nodes > Nodes || depth > Depth;
        }

        bool Crossed() const
        {
            if (!Timed) return false;

            return Elapsed() > ActualTime;
        }

    };

    template<SearchThreadType ThreadType = Main, class EventHandler = DefaultSearchEventHandler>
    class SearchThread
    {

        static_assert(
            ThreadType != Parallel || std::is_same_v<EventHandler, DefaultSearchEventHandler>,
            "Events are only called in the main thread, this must be a mistake"
        );

        static_assert(
            std::is_base_of_v<DefaultSearchEventHandler, EventHandler>,
            "EventHandler must be derived from DefaultSearchEventHandler"
        );

        Board Board {};

        KTable Killer  {};
        HTable History {};

        SearchStack Stack {};

        RepetitionStack Repetition {};

        PVTable PVTable {};

        SearchThreadStatus Status = Stopped;

        Limit Limit {};

        uint8_t SelectiveDepth = 0;

         int16_t  IDepth = 0;
        uint64_t   Nodes = 0;
        uint64_t TTNodes = 0;

        Score Evaluation = -Infinity;

        Move BestMove {};

        public:
        SearchThread() = delete;

        // ReSharper disable CppPassValueParameterByConstReference
        SearchThread(
            const StockDory::Limit      limit,
            const StockDory::Board      board,
            const RepetitionStack  repetition,
            const uint8_t                 hmc)
        : Board(board), Repetition(repetition), Limit(limit)
        {
            Stack[0].HalfMoveCounter = hmc;
        }
        // ReSharper restore CppPassValueParameterByConstReference

        void IterativeDeepening()
        {
            // TODO: Handle Evaluation Parallelism
            Board.LoadForEvaluation();

            IDepth = 1;
            while (!Limit.Crossed(Nodes, IDepth)) {
                const Move lastBestMove = BestMove;

                if (Board.ColorToMove() ==   White)
                     Evaluation = Aspiration<White>(IDepth);
                else Evaluation = Aspiration<Black>(IDepth);

                if (Status == SearchThreadStatus::Stopped) break;

                if (ThreadType == Main) {
                    BestMove = PVTable[0].PV[0];

                    EventHandler::HandleDepthIteration(
                        IDepth,
                        SelectiveDepth,
                        Evaluation,
                        Nodes,
                        TTNodes,
                        Limit.Elapsed()
                    );
                }

                IDepth++;
            }

            Status = SearchThreadStatus::Stopped;

            if (ThreadType == Main) EventHandler::HandleBestMove(BestMove);
        }

        void Stop() { Status = SearchThreadStatus::Stopped; }

        bool Stopped() const { return Status == SearchThreadStatus::Stopped; }

        private:
        template<Color Color>
        Score Aspiration(const int16_t depth)
        {
            // Set the aspiration window size

            Score alpha = -Infinity;
            Score beta  =  Infinity;

            if (depth > AspirationWindowRequiredDepth) {
                // If we're past the required aspiration depth, it means previous searches
                // gave us a good ballpark for the evaluation. All future searches can start
                // with a much smaller window:
                //
                // [evaluation - AspirationWindowSize, evaluation + AspirationWindowSize]

                alpha = Evaluation - AspirationWindowSize;
                beta  = Evaluation + AspirationWindowSize;
            }

            uint8_t research = 0;
            while (true) {
                if (ThreadType == Main) {
                    // The main thread is also responsible for checking if we've crossed any time limits.
                    // If we have, we need to stop searching
                    if (Limit.Crossed()) [[unlikely]] { Status = SearchThreadStatus::Stopped; return Draw; }
                }

                // If the search was stopped, we need to stop searching.
                // This search is incomplete, and its results may not be valid to use
                if (Status == SearchThreadStatus::Stopped) return Draw;

                // If the previous searches weren't successful, and even incrementally
                // widening the window slightly didn't help, we need to widen the window to
                // a full window and avoid wasting any more time
                if (alpha < -AspirationWindowFallbackBound) alpha = -Infinity;
                if (beta  >  AspirationWindowFallbackBound) beta  =  Infinity;

                const Score bestEvaluation = AlphaBeta<Color, true, true>(0, depth, alpha, beta);

                if        (bestEvaluation <= alpha) {
                    // If the evaluation from this search is far below the lower bound, it means that the
                    // previous searches were too optimistic. We need to widen the window a bit and try again

                    research++;
                    alpha = std::max<Score>(alpha - research * research * AspirationWindowSizeDelta, -Infinity);
                } else if (bestEvaluation >= beta) {
                    // If the evaluation from this search is far above the upper bound, it means that the
                    // previous searches were too pessimistic. We need to widen the window a bit and try again

                    research++;
                    beta  = std::min<Score>(beta  + research * research * AspirationWindowSizeDelta,  Infinity);

                    // We should also assume that this is the best move so far
                    BestMove = PVTable[0].PV[0];
                } else {
                    // If the evaluation from this search falls within the window, we can assume that the
                    // search was successful

                    return bestEvaluation;
                }
            }
        }

        template<Color Color, bool PV, bool Root>
        Score AlphaBeta(const uint8_t ply, const int16_t depth, const Score alpha, const Score beta)
        {

        }

    };

    inline std::vector<std::shared_ptr<SearchThread<Parallel>>> ParallelThreads;

    // It's important to call this and check to make sure that all threads are stopped
    // before starting a new search
    bool SafeToSearch()
    {
        for (const auto& thread : ParallelThreads) if (!thread->Stopped()) return false;

        return true;
    }

    template<class EventHandler = DefaultSearchEventHandler>
    std::unique_ptr<SearchThread<Main, EventHandler>> Search(
              Limit          &      limit,
              Board          &      board,
              RepetitionStack& repetition,
        const uint8_t                 hmc)
    {
        limit.Start();

        const size_t parallelThreadCount = ThreadPool.Size() - 1;

        // We don't need to clear since the previous search will have cleared the previous threads
        ParallelThreads.resize(parallelThreadCount);

        // Allocate and start the parallel threads
        for (size_t i = 0; i < parallelThreadCount; i++) ThreadPool.Execute(
            [i, &limit, &board, &repetition, hmc] -> void
            {
                const auto thread = std::make_shared<SearchThread<Parallel>>(limit, board, repetition, hmc);
                ParallelThreads[i] = thread;

                thread->IterativeDeepening();
            }
        );

        // Allocate the main thread
        auto main = std::make_unique<SearchThread<Main, EventHandler>>(limit, board, repetition, hmc);

        // Start the main thread
        ThreadPool.Execute(
            [&main, parallelThreadCount] -> void
            {
                main->IterativeDeepening();

                // Once the main thread is done, stop all parallel threads
                for (const auto& thread : ParallelThreads) thread->Stop();

                // Wait for all parallel threads to finish
                for (size_t i = 0; i < parallelThreadCount; i++) {
                    const auto& thread = ParallelThreads[i];

                    while (!thread->Stopped()) { Sleep(5); }
                }

                // Clear the parallel threads so that we can safely start a new search
                ParallelThreads.clear();
            }
        );

        // Return a reference to the main thread
        // This is so that the main thread can be used to stop the search
        return main;
    }

} // StockDory

#endif //STOCKDORY_SEARCH2_H
