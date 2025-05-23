//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEARCH2_H
#define STOCKDORY_SEARCH2_H

#include "../Backend/Board.h"
#include "../Backend/Misc.h"
#include "../Backend/Type/Move.h"
#include "../Backend/ThreadPool.h"

#include "Common.h"
#include "TranspositionTable.h"

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

    template<bool Timed>
    struct Limit
    {

        uint64_t Nodes = std::numeric_limits<uint64_t>::max();
        uint8_t  Depth = MaxDepth / 2;

        TP Origin = std::chrono::steady_clock::now();

        std::conditional_t<Timed, MS, std::monostate> ActualTime  {};
        std::conditional_t<Timed, MS, std::monostate> OptimalTime {};

        void Start() { Origin = std::chrono::steady_clock::now(); }

        MS Elapsed() const
        {
            return std::chrono::duration_cast<MS>(std::chrono::steady_clock::now() - Origin);
        }

        [[clang::always_inline]]
        bool Crossed(const uint64_t nodes, const uint8_t depth) const requires (!Timed)
        {
            return nodes > Nodes || depth > Depth;
        }

        template<bool Hard = false>
        [[clang::always_inline]]
        bool Crossed(const uint64_t nodes, const uint8_t depth) const requires (Timed)
        {
            if (Hard) return Elapsed() > ActualTime;

            return Elapsed() > OptimalTime || nodes > Nodes || depth > Depth;
        }

    };

    template<SearchThreadType ThreadType = Main, bool Timed = false, class EventHandler = DefaultSearchEventHandler>
    class SearchThread
    {

        static_assert(
            ThreadType != Main && std::is_same_v<EventHandler, DefaultSearchEventHandler>,
            "Events are only called in the main thread, this must be a mistake"
        );

        static_assert(
            ThreadType != Main && Timed,
            "Timing search is only appropriate on the main thread, this must be a mistake"
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

        Limit<Timed> Limit {};

        uint8_t SelectiveDepth = 0;

         int16_t  IDepth = 0;
        uint64_t   Nodes = 0;
        uint64_t TTNodes = 0;

        Score Evaluation = -Infinity;

        Move BestMove {};

        public:
        SearchThread() = default;

        // ReSharper disable CppPassValueParameterByConstReference
        SearchThread(
            const Limit<Timed>          limit,
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

                // TODO: Start Aspiration Search

                if (Status == Stopped) break;

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

            if (ThreadType == Main) EventHandler::HandleBestMove(BestMove);
        }

        void Stop() { Status = Stopped; }

    };

    inline std::vector<SearchThread<Parallel>> ParallelThreads;

    template<class EventHandler = DefaultSearchEventHandler, bool Timed = false>
    std::unique_ptr<SearchThread<Main, Timed, EventHandler>>& Search(
        const Limit<Timed>   &      limit,
        const Board          &      board,
        const RepetitionStack& repetition,
        const uint8_t                 hmc)
    {
        limit.Start();

        // Allocate the parallel threads
        ParallelThreads.reserve(ThreadPool.Size() - 1);

        // Start the parallel threads
        for (size_t i = 0; i < ThreadPool.Size() - 1; i++) ThreadPool.Execute(
            [i, &limit, &board, &repetition, hmc] -> void
            {
                ParallelThreads[i] = SearchThread<Parallel>(limit, board, repetition, hmc);
                ParallelThreads[i].IterativeDeepening();
            }
        );

        // Allocate the main thread
        auto main = std::make_unique<SearchThread<Main, Timed, EventHandler>>(limit, board, repetition, hmc);

        // Start the main thread
        ThreadPool.Execute(
            [&main] -> void
            {
                main->IterativeDeepening();

                // Once the main thread is done, stop all parallel threads
                for (auto& thread : ParallelThreads) thread.Stop();
            }
        );

        // Return a reference to the main thread
        // This is so that the main thread can be used to stop the search
        return main;
    }

} // StockDory

#endif //STOCKDORY_SEARCH2_H
