//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEARCH2_H
#define STOCKDORY_SEARCH2_H

#include <atomic>

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

    struct IterativeDeepeningIterationCompletionEvent
    {

        int16_t           Depth {};
        uint8_t  SelectiveDepth {};
        Score        Evaluation {};
        uint64_t          Nodes {};
        MS                 Time {};

    };

    struct IterativeDeepeningCompletionEvent
    {

        Move Move {};

    };

    struct DefaultSearchEventHandler
    {

        // ReSharper disable once CppMemberFunctionMayBeStatic
        void HandleIterativeDeepeningIterationCompletion(
            [[maybe_unused]] const IterativeDeepeningIterationCompletionEvent& event
        ) {}

        // ReSharper disable once CppMemberFunctionMayBeStatic
        void HandleIterativeDeepeningCompletion(
            [[maybe_unused]] const IterativeDeepeningCompletionEvent& event
        ) {}

    };

    template<SearchThreadType ThreadType = Main, class EventHandler = DefaultSearchEventHandler>
    class SearchTask
    {

        static_assert(
            std::is_base_of_v<DefaultSearchEventHandler, EventHandler>,
            "Handler must be derived from DefaultSearchEventHandler"
        );

        Board Board {};

        KTable Killer  {};
        HTable History {};

        SearchStack Stack {};

        RepetitionStack Repetition {};

        PVTable PVTable {};

        SearchThreadStatus Status = SearchThreadStatus::Stopped;

        Limit Limit {};

        uint8_t SelectiveDepth = 0;

        int16_t IDepth = 0;

        std::atomic_uint64_t Nodes = 0;

        Score Evaluation = -Infinity;

        Move BestMove {};

        EventHandler Handler {};

        public:
        SearchTask() = default;

        // ReSharper disable CppPassValueParameterByConstReference
        SearchTask(
            const StockDory::Limit      limit,
            const StockDory::Board      board,
            const RepetitionStack  repetition,
            const uint8_t                 hmc,
                  EventHandler&       handler)
        : Board(board), Repetition(repetition), Limit(limit), Handler(handler)
        {
            Stack[0].HalfMoveCounter = hmc;
        }
        // ReSharper restore CppPassValueParameterByConstReference

        uint64_t GetNodes() const { return Nodes; }

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

                    IterativeDeepeningIterationCompletionEvent event
                    {
                        .Depth          = IDepth,
                        .SelectiveDepth = SelectiveDepth,
                        .Evaluation     = Evaluation,
                        .Nodes          = Nodes,
                        .Time           = Limit.Elapsed()
                    };

                    Handler.HandleIterativeDeepeningIterationCompletion(event);
                }

                IDepth++;
            }

            Status = SearchThreadStatus::Stopped;

            if (ThreadType == Main) {
                IterativeDeepeningCompletionEvent event
                {
                    .Move = BestMove
                };

                Handler.HandleIterativeDeepeningCompletion(event);
            }
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
                    if (Limit.Crossed()) [[unlikely]] { Status = SearchThreadStatus::Stopped; }
                }

                // If the search was stopped, we need to stop searching.
                // This search is incomplete, and its results may not be valid to use
                if (Status == SearchThreadStatus::Stopped) [[unlikely]] return Draw;

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
        Score AlphaBeta(const uint8_t ply, int16_t depth, Score alpha, Score beta)
        {
            constexpr auto OColor = Opposite(Color);

            if (ThreadType == Main) {
                // The main thread is responsible for checking if we've crossed any time limits.
                // If we have, we need to stop searching
                if (Limit.Crossed()) [[unlikely]] { Status = SearchThreadStatus::Stopped; }
            }

            // If the search was stopped, we need to stop searching
            if (Status == SearchThreadStatus::Stopped) [[unlikely]] return Draw;

            if (ThreadType == Main) {
                // The main thread is responsible for maintaining the correct PV, the parallel threads
                // need not worry about this
                PVTable[ply].Ply = ply;

                // Only the main thread ever outputs the selective depth. No reason to worry about it on
                // the parallel threads
                if (PV) SelectiveDepth = std::max(SelectiveDepth, ply);
            }

            // If we've exhausted the search depth, we should evaluate tactical positions a bit deeper
            // to avoid the horizon effect
            if (depth <= 0) return Quiescence<Color, PV>(ply, alpha, beta);

            const ZobristHash hash = Board.Zobrist();

            if (!Root) {
                // If we're not at the root, we need to check for potential draws so we don't
                // accidentally consider them as wins or losses

                // If the half-move counter is greater than 100, it means at least 50 moves have been
                // played without a pawn move or capture. This is a draw, according to the 50-move rule,
                // and while it relies on the opponent to claim the draw, we should still assume the
                // opponent will do so and consider this position as a draw
                if (Stack[ply].HalfMoveCounter >= 100) return Draw;

                // If the position has been repeated enough times (3 times), we should also consider this a draw
                // as per the 3-fold repetition rule
                if (Repetition.Found(hash, Stack[ply].HalfMoveCounter)) return Draw;

                const uint8_t pieceCount = Count(~Board[NAC]);

                // If there are only two pieces left on the board, that would be our king and the opponent's king,
                // and that's a draw. We can never checkmate the opponent with only a king left
                if (pieceCount == 2) return Draw;

                const bool knightLeft = Board.PieceBoard<White>(Knight) | Board.PieceBoard<Black>(Knight),
                           bishopLeft = Board.PieceBoard<White>(Bishop) | Board.PieceBoard<Black>(Bishop);

                // If there are only 3 pieces left on the board, then two of them must be the kings, and the third
                // can be any piece. In the case it's a knight or a bishop, we can never checkmate the opponent
                if (pieceCount == 3 && (knightLeft || bishopLeft)) return Draw;

                // Mate Distance Pruning:
                // Even if we mate the opponent in the next move, our score would be at best: Mate - ply - 1
                // but if we have alpha greater than or equal to that, it means a shorter mate was found at
                // some earlier ply. There is no need to search this branch any further since we can't find
                // a shallower mate at this depth, so we can return the alpha value
                alpha = std::max<Score>(alpha, -Mate + ply    );
                beta  = std::min<Score>(beta ,  Mate - ply - 1);
                if (alpha >= beta) return alpha;
            }

            const SearchTranspositionEntry ttEntry = TT[hash];
                  Move                     ttMove  = {};
                  bool                     ttHit   = false;

            // Check if the transposition table has an entry for this position and if it does,
            // check if the entry is valid by comparing the bits of the hash not used to index
            // the table
            if (ttEntry.Type != Invalid && ttEntry.Hash == CompressHash(hash)) {
                ttHit  = true;
                ttMove = ttEntry.Move;

                // We only return from the transposition table in non-PV nodes. In PV nodes,
                // returning from the transposition table is too risky since hash collisions
                // can happen, and we might return a wrong value. In non-PV nodes, we can do
                // this since while hash collisions can occur, they are rare and at most they
                // will just influence the search order and not the final evaluation. We should
                // also check that the entry came from a search that was at least as deep as the
                // current planned search
                if (!PV && ttEntry.Depth >= depth) {
                    if (ttEntry.Type == Exact                               ) return ttEntry.Evaluation;
                    if (ttEntry.Type == Beta  && ttEntry.Evaluation >= beta ) return ttEntry.Evaluation;
                    if (ttEntry.Type == Alpha && ttEntry.Evaluation <= alpha) return ttEntry.Evaluation;
                }
            }

            Score staticEvaluation = None ;
            bool  improving               ;

            const bool checked = Board.Checked<Color>();

            if (checked) {
                // If we're in check, we can't trust the static evaluation of this node
                // and should use the static evaluation of our previous node since it
                // will be more reasonable

                staticEvaluation = Stack[ply - 2].StaticEvaluation;

                // Getting into check typically means we're not improving - this is not
                // ground truth, but it's a good estimate for heuristics
                improving = false;

                // We should search this branch a bit deeper since we need to find a good
                // out of check
                depth += CheckExtension;

                // We should also skip all the risky pruning since we need to be careful
                // not to prune the only good move that gets us out of check
                goto SkipRiskyPruning;
            }

            // Check if we have a valid transposition table entry. If we do, we can
            // use it instead of the neural network evaluation since it will most
            // likely be more accurate
            if (ttHit) {
                staticEvaluation = ttEntry.Evaluation;

                // If the entry is not an exact evaluation (it came from a non-PV node),
                // it is likely that the evaluation is not accurate enough
                if (ttEntry.Type != Exact) {
                    const Score nnEvaluation = Evaluation::Evaluate(Color);

                    // If the neural network evaluation further exceeds the estimate from the
                    // transposition table, we can use it instead. This is a good estimat
                    if ((nnEvaluation > ttEntry.Evaluation && ttEntry.Type == Beta )  ||
                        (nnEvaluation < ttEntry.Evaluation && ttEntry.Type == Alpha)) staticEvaluation = nnEvaluation;
                }
            }

            Stack[ply].StaticEvaluation = staticEvaluation;

            // We are improving our position if the static evaluation is increasing - this is
            // not ground truth, but it's a good estimate for heuristics
            improving = Stack[ply].StaticEvaluation > Stack[ply - 2].StaticEvaluation;

            // Reverse Futility Pruning
            if (depth < ReverseFutilityDisablingDepth && abs(beta) < Mate) {
                const Score margin = depth * ReverseFutilityDepthFactor + improving * ReverseFutilityImprovingFactor;

                if (staticEvaluation - margin >= beta) return beta;
            }

            SkipRiskyPruning:
        }

        template<Color Color, bool PV>
        Score Quiescence(const uint8_t ply, Score alpha, Score beta)
        {

        }

    };

    template<class EventHandler = DefaultSearchEventHandler>
    class Search
    {

        using ParallelSearchTask = SearchTask<Parallel>;

        struct MainSearchTaskEventHandler : DefaultSearchEventHandler
        {

            EventHandler MainHandler;

            std::vector<std::unique_ptr<ParallelSearchTask>> ParallelTasks;

            void HandleIterativeDeepeningIterationCompletion(
                [[maybe_unused]] const IterativeDeepeningIterationCompletionEvent& event
            )
            {
                IterativeDeepeningIterationCompletionEvent mainEvent
                {
                    .Depth          = event.Depth,
                    .SelectiveDepth = event.SelectiveDepth,
                    .Evaluation     = event.Evaluation,
                    .Nodes          = event.Nodes,
                    .Time           = event.Time
                };

                for (const auto& task : ParallelTasks) mainEvent.Nodes += task->GetNodes();

                MainHandler.HandleIterativeDeepeningIterationCompletion(mainEvent);
            }

            void HandleIterativeDeepeningCompletion(
                [[maybe_unused]] const IterativeDeepeningCompletionEvent& event
            )
            { MainHandler.HandleIterativeDeepeningCompletion(event); }

        };

        std::atomic_bool Searching = false;

        using MainSearchTask = SearchTask<Main, MainSearchTaskEventHandler>;

        MainSearchTask              MainTask       ;
        MainSearchTaskEventHandler  MainTaskHandler;

        public:
        Search(EventHandler& handler)
        {
            MainTaskHandler.MainHandler = handler;
        }

        void Run(
                  Limit          &      limit,
                  Board          &      board,
                  RepetitionStack& repetition,
            const uint8_t                 hmc)
        {

            Searching = true;

            limit.Start();

            const size_t freeThreadCount = ThreadPool.Size() - 1;

            // Allocate and start the parallel tasks
            MainTaskHandler.ParallelTasks.resize(freeThreadCount);

            for (size_t i = 0; i < freeThreadCount; i++) ThreadPool.Execute(
                [this, i, &limit, &board, &repetition, hmc] -> void
                {
                    DefaultSearchEventHandler handler;
                    auto task = std::make_unique<ParallelSearchTask>(limit, board, repetition, hmc, handler);

                    MainTaskHandler.ParallelTasks[i] = std::move(task);

                    task->IterativeDeepening();
                }
            );

            // Allocate the main task
            MainTask = MainSearchTask(limit, board, repetition, hmc, MainTaskHandler);

            // Start the main task
            ThreadPool.Execute(
                [this, freeThreadCount] -> void
                {
                    MainTask->IterativeDeepening();

                    // Once the main task is done, stop all parallel tasks
                    for (const auto& task : MainTaskHandler.ParallelTasks) task->Stop();

                    // Wait for all parallel tasks to finish
                    for (size_t i = 0; i < freeThreadCount; i++) {
                        const auto& task = MainTaskHandler.ParallelTasks[i];

                        while (!task->Stopped()) { Sleep(1); }
                    }

                    Searching = false;
                }
            );
        }

        void Stop()
        {
            MainTask.Stop();
        }

    };

} // StockDory

#endif //STOCKDORY_SEARCH2_H
