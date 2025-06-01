//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEARCH_H
#define STOCKDORY_SEARCH_H

#include <cmath>

#include "../Backend/Board.h"
#include "../Backend/Misc.h"
#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Move.h"

#include "OrderedMoveList.h"

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

    auto LMRReductionTable =
    [] -> Array<int16_t, MaxDepth, MaxMove>
    {
        const auto formula = [](const uint8_t depth, const uint8_t move) -> int16_t
        {
            return static_cast<int16_t>(std::log(depth) * std::log(move) / 2 - 0.2);
        };

        Array<int16_t, MaxDepth, MaxMove> temp {};

        for (uint8_t depth = 1; depth < MaxDepth; depth++)
        for (uint8_t move  = 1; move  < MaxMove ;  move++) temp[depth][move] = formula(depth, move);

        return temp;
    }();

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
              Frame& operator[](const size_t index)       { return Internal[index + Padding]; }
        const Frame& operator[](const size_t index) const { return Internal[index + Padding]; }

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
            uint8_t checked = 0, found = 0;
            for (uint16_t i = CurrentIndex - 1; i != 0xFFFF; i--) {
                if (checked > halfMoveCounter) break;

                if (found == 2 && Internal[i] == hash) return true;

                found += Internal[i] == hash;
                checked++;
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
        bool Fixed = false;

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
            const auto result = nodes > Nodes || depth > Depth;

            return Timed ? Elapsed() > OptimalTime || result : result;
        }

        bool Crossed() const { return Timed ? Elapsed() > ActualTime : false; }

    };

    class WDLCalculator
    {

        constexpr static double Scale = 1000.0;

        struct Coefficient { double A = 0.0; double B = 0.0; };

        enum Weight : uint8_t { A, B };

        constexpr static Array<double, 2, 4> WValue = {{
            {  190.541, -461.949,  185.975,  334.376 },
            {  106.738, -285.539,  297.152, -  9.669 }
        }};

        template<Weight W>
        [[clang::always_inline]]
        static double Formula(const Score x)
        {
            return ((WValue[W][0] * x / 58 + WValue[W][1]) * x / 58 + WValue[W][2]) * x / 58 + WValue[W][3];
        }

        [[clang::always_inline]]
        static Coefficient Coefficient(const Board& board)
        {
            const BitBoard pawn   = board.PieceBoard(Pawn  , White) | board.PieceBoard(Pawn  , Black);
            const BitBoard knight = board.PieceBoard(Knight, White) | board.PieceBoard(Knight, Black);
            const BitBoard bishop = board.PieceBoard(Bishop, White) | board.PieceBoard(Bishop, Black);
            const BitBoard rook   = board.PieceBoard(Rook  , White) | board.PieceBoard(Rook  , Black);
            const BitBoard queen  = board.PieceBoard(Queen , White) | board.PieceBoard(Queen , Black);

            const Score mat = Count(pawn  ) * 1 +
                              Count(knight) * 3 +
                              Count(bishop) * 3 +
                              Count(rook  ) * 5 +
                              Count(queen ) * 9 ;

            return { Formula<A>(mat), Formula<B>(mat) };
        }

        public:
        [[clang::always_inline]]
        static Score W(const Board& board, const Score cp)
        {
            const auto [a, b] = Coefficient(board);

            return round(Scale / (1 + exp((a - cp) / b)));
        }

        [[clang::always_inline]]
        static Score L(const Board& board, const Score cp)
        {
            return W(board, -cp);
        }

        [[clang::always_inline]]
        static Score D(const Board& board, const Score cp) { return Scale - W(board, cp) - L(board, cp); }

        [[clang::always_inline]]
        static Score S(const Board& board, const Score cp)
        {
            if (cp == 0 || abs(cp) >= Mate - MaxDepth) return cp;

            const auto [a, b] = Coefficient(board);

            return round((Scale / 10) * cp / a);
        }

    };

    struct WDL
    {

        Score W; Score D; Score L;

        WDL() : W(0), D(1000), L(0) {}

        [[clang::always_inline]]
        WDL(const Board& board, const Score cp) : W(WDLCalculator::W(board, cp)),
                                                  D(WDLCalculator::D(board, cp)),
                                                  L(WDLCalculator::L(board, cp)) {}

    };

    struct IterativeDeepeningIterationCompletionEvent
    {

        int16_t           Depth {};
        uint8_t  SelectiveDepth {};
        Score        Evaluation {};
        WDL                 WDL {};
        uint64_t          Nodes {};
        MS                 Time {};
        PVEntry         PVEntry {};

    };

    struct IterativeDeepeningCompletionEvent
    {

        Move Move {};

    };

    struct DefaultSearchEventHandler
    {

        static void HandleIterativeDeepeningIterationCompletion(const IterativeDeepeningIterationCompletionEvent& _)
        {}

        static void HandleIterativeDeepeningCompletion(const IterativeDeepeningCompletionEvent& _)
        {}

    };

    template<SearchThreadType ThreadType = Main, class EventHandler = DefaultSearchEventHandler>
    class SearchTask
    {

        Board Board {};

        KTable Killer  {};
        HTable History {};

        SearchStack Stack {};

        RepetitionStack Repetition {};

        PVTable PVTable {};

        Limit Limit {};

        uint8_t SelectiveDepth = 0;

        int16_t IDepth = 0;

        uint64_t Nodes = 0;

        Score Evaluation = -Infinity;

        Move BestMove {};

        uint8_t SearchStability = 0;

        size_t ThreadId = 0;

        SearchThreadStatus Status = Running;

        public:
        SearchTask() {}

        // ReSharper disable CppPassValueParameterByConstReference
        SearchTask(
            const StockDory::Limit      limit,
            const StockDory::Board      board,
            const RepetitionStack  repetition,
            const uint8_t                 hmc,
            const size_t             threadId = 0)
        : Board(board), Repetition(repetition), Limit(limit), ThreadId(threadId)
        {
            Stack[0].HalfMoveCounter = hmc;
        }
        // ReSharper restore CppPassValueParameterByConstReference

        uint64_t GetNodes() const { return Nodes; }

        void IterativeDeepening()
        {
            if (ThreadType == Main) {
                SearchSingleMoveTimeOptimization();

                Limit.Start();
            }

            Board.LoadForEvaluation(ThreadId);

            IDepth = 1;
            while (!Limit.Crossed(Nodes, IDepth)) {
                const Move lastBestMove = BestMove;

                if (Board.ColorToMove() ==   White)
                     Evaluation = Aspiration<White>(IDepth);
                else Evaluation = Aspiration<Black>(IDepth);

                if (Status == SearchThreadStatus::Stopped) break;

                if (ThreadType == Main) {
                    const auto time = Limit.Elapsed();

                    BestMove = PVTable[0].PV[0];

                    SearchStabilityTimeOptimization(lastBestMove);

                    EventHandler::HandleIterativeDeepeningIterationCompletion({
                        .Depth          = IDepth,
                        .SelectiveDepth = SelectiveDepth,
                        .Evaluation     = WDLCalculator::S(Board, Evaluation),
                        .WDL            = WDL(Board, Evaluation),
                        .Nodes          = Nodes,
                        .Time           = time,
                        .PVEntry        = PVTable[0]
                    });
                }

                IDepth++;
            }

            Status = SearchThreadStatus::Stopped;

            if (ThreadType == Main) {
                EventHandler::HandleIterativeDeepeningCompletion({
                    .Move = BestMove
                });
            }
        }

        void Stop() { Status = SearchThreadStatus::Stopped; }

        bool Stopped() const { return Status == SearchThreadStatus::Stopped; }

        Score GetEvaluation() const { return WDLCalculator::S(Board, Evaluation); }

        WDL GetWDL() const { return WDL(Board, Evaluation); }

        private:
        void SearchSingleMoveTimeOptimization()
        {
            if (!Limit.Timed) return;
            if ( Limit.Fixed) return;

            uint8_t moveCount;

            if (Board.ColorToMove() == White) {
                const OrderedMoveList<White> moves (Board, 0, Killer, History);
                moveCount = moves.Count();
            } else {
                const OrderedMoveList<Black> moves (Board, 0, Killer, History);
                moveCount = moves.Count();
            }

            if (moveCount > 1) return;

            const uint64_t time = Limit.OptimalTime.count();

            Limit.OptimalTime = MS(time * TimeBasePartitionNumerator / TimeBasePartitionDenominator);
        }

        void SearchStabilityTimeOptimization(const Move lastBestMove)
        {
            if (!Limit.Timed) return;
            if ( Limit.Fixed) return;

            if (lastBestMove == BestMove) SearchStability = std::min<uint8_t>(SearchStability + 1, 4);
            else                          SearchStability = 0;

            const auto factor = SearchStabilityTimeOptimizationFactor[SearchStability];

            const uint64_t time = Limit.OptimalTime.count();

            Limit.OptimalTime = MS(std::min<uint64_t>(time * factor / 100, Limit.ActualTime.count()));
        }

        template<Color Color>
        Score Aspiration(const int16_t depth)
        {
            Score alpha = -Infinity;
            Score beta  =  Infinity;

            if (depth >= AspirationWindowMinimumDepth) {
                alpha = Evaluation - AspirationWindowSize;
                beta  = Evaluation + AspirationWindowSize;
            }

            uint8_t research = 0;
            while (true) {
                if (ThreadType == Main) {
                    if (Limit.Crossed()) [[unlikely]] { Status = SearchThreadStatus::Stopped; }
                }

                if (Status == SearchThreadStatus::Stopped) [[unlikely]] return Draw;

                if (alpha < -AspirationWindowFallbackBound) alpha = -Infinity;
                if (beta  >  AspirationWindowFallbackBound) beta  =  Infinity;

                const Score bestEvaluation = AlphaBeta<Color, true, true>(0, depth, alpha, beta);

                if        (bestEvaluation <= alpha) {
                    research++;
                    alpha = std::max<Score>(alpha - research * research * AspirationWindowSizeDelta, -Infinity);
                } else if (bestEvaluation >= beta) {
                    research++;
                    beta  = std::min<Score>(beta  + research * research * AspirationWindowSizeDelta,  Infinity);

                    BestMove = PVTable[0].PV[0];
                } else return bestEvaluation;
            }
        }

        template<Color Color, bool PV, bool Root>
        Score AlphaBeta(const uint8_t ply, int16_t depth, Score alpha, Score beta)
        {
            constexpr auto OColor = Opposite(Color);

            if (ThreadType == Main) {
                if ((Nodes & 4095) == 0 && Limit.Crossed()) [[unlikely]] { Status = SearchThreadStatus::Stopped; }
            }

            if (Status == SearchThreadStatus::Stopped) [[unlikely]] return Draw;

            if (ThreadType == Main) {
                PVTable[ply].Ply = ply;

                if (PV) SelectiveDepth = std::max(SelectiveDepth, ply);
            }

            if (depth <= 0) return Quiescence<Color, PV>(ply, alpha, beta);

            const ZobristHash hash = Board.Zobrist();

            if (!Root) {
                if (Stack[ply].HalfMoveCounter >= 100) return Draw;

                if (Repetition.Found(hash, Stack[ply].HalfMoveCounter)) return Draw;

                const uint8_t pieceCount = Count(~Board[NAC]);

                if (pieceCount == 2) return Draw;

                const bool knightLeft = Board.PieceBoard<White>(Knight) | Board.PieceBoard<Black>(Knight),
                           bishopLeft = Board.PieceBoard<White>(Bishop) | Board.PieceBoard<Black>(Bishop);

                if (pieceCount == 3 && (knightLeft || bishopLeft)) return Draw;

                alpha = std::max<Score>(alpha, -Mate + ply    );
                beta  = std::min<Score>(beta ,  Mate - ply - 1);
                if (alpha >= beta) return alpha;
            }

            SearchTranspositionEntry& ttEntry = TT[hash];
            Move                      ttMove  = {};
            bool                      ttHit   = false;

            if (ttEntry.Type != Invalid && ttEntry.Hash == CompressHash(hash)) {
                ttHit  = true;
                ttMove = ttEntry.Move;

                if (!PV && ttEntry.Depth >= depth) {
                    if (ttEntry.Type == Exact                               ) return ttEntry.Evaluation;
                    if (ttEntry.Type == Beta  && ttEntry.Evaluation >= beta ) return ttEntry.Evaluation;
                    if (ttEntry.Type == Alpha && ttEntry.Evaluation <= alpha) return ttEntry.Evaluation;
                }
            }

            Score staticEvaluation;
            bool  improving       ;

            const bool checked = Board.Checked<Color>();

            if (checked) {
                staticEvaluation = Stack[ply].StaticEvaluation = Stack[ply - 2].StaticEvaluation;

                improving = false;

                depth += CheckExtension;

                goto SkipRiskyPruning;
            }

            if (ttHit) {
                staticEvaluation = ttEntry.Evaluation;

                if (ttEntry.Type != Exact) {
                    const Score nnEvaluation = Evaluation::Evaluate(Color, ThreadId);

                    if      (ttEntry.Type == Beta ) staticEvaluation = std::max<Score>(staticEvaluation, nnEvaluation);
                    else if (ttEntry.Type == Alpha) staticEvaluation = std::min<Score>(staticEvaluation, nnEvaluation);
                }
            } else staticEvaluation = Evaluation::Evaluate(Color, ThreadId);

            Stack[ply].StaticEvaluation = staticEvaluation;

            improving = staticEvaluation > Stack[ply - 2].StaticEvaluation;

            if (!PV) {
                if (depth < ReverseFutilityMaximumDepth && abs(beta) < Mate) {
                    const Score     depthMargin = depth     * ReverseFutilityDepthFactor;
                    const Score improvingMargin = improving * ReverseFutilityImprovingFactor;

                    if (staticEvaluation - depthMargin + improvingMargin >= beta) return beta;
                }

                if (depth == RazoringDepth && staticEvaluation + RazoringEvaluationMargin < alpha)
                    return Quiescence<Color, false>(ply, alpha, beta);

                if (!Root && depth >= NullMoveMinimumDepth && staticEvaluation >= beta) {
                    const auto ScalingDepthReduction      = depth / NullMoveDepthFactor;
                    const auto ScalingEvaluationReduction = std::min<int16_t>(
                        (staticEvaluation - beta) / NullMoveEvaluationFactor,
                        NullMoveMinimumReduction
                    );

                    const int16_t reducedDepth =
                          depth
                        - (  NullMoveMinimumReduction
                          +  ScalingDepthReduction
                          +  ScalingEvaluationReduction
                          );

                    const PreviousStateNull state = Board.Move();

                    const auto evaluation = -AlphaBeta<OColor, false, false>(
                        ply + 1,
                        reducedDepth,
                        -beta,
                        -beta + 1
                    );

                    Board.UndoMove(state);

                    if (evaluation >= beta) return beta;
                }
            }

            SkipRiskyPruning:

            if (depth >= IIRMinimumDepth && !ttHit) depth -= IIRDepthReduction;

            using MoveList = OrderedMoveList<Color>;

            MoveList moves (Board, ply, Killer, History, ttMove);

            if (moves.Count() == 0) return checked ? -Mate + ply : 0;

            SearchTranspositionEntry ttEntryNew
            {
                .Hash       = CompressHash(hash),
                .Evaluation = -CompressedInfinity,
                .Move       = ttMove,
                .Depth      = static_cast<uint8_t>(depth),
                .Type       = Alpha
            };

            const uint8_t lmpLastQuiet = LMPLastQuietBase +   depth * depth;
            const bool    doLMP        = !Root && !checked && depth <= LMPMaximumDepth;
            const bool    doLMR        =          !checked && depth >= LMRMinimumDepth;

            Score bestEvaluation = -Infinity;

            uint8_t quietMoves = 0;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move  = moves[i];
                const bool quiet = Board[move.To()].Piece() == NAP;

                quietMoves += quiet;

                if (i > 0 && quiet) {
                    const Score margin = depth * FutilityDepthFactor;

                    if (staticEvaluation + margin <= alpha) break;
                }

                if (!PV) {
                    if (doLMP && quietMoves > lmpLastQuiet && bestEvaluation > -Infinity) break;
                }

                const PreviousState state = DoMove<true>(move, ply, quiet);

                Score evaluation = 0;

                if (i == 0) evaluation = -AlphaBeta<OColor, PV, false>(ply + 1, depth - 1, -beta, -alpha);
                else {
                    if (doLMR && i > LMRMinimumMoves) {
                        int16_t r = LMRReductionTable[depth][i];

                        if (!PV) r++;

                        if (!improving) r++;

                        if (Board.Checked<OColor>()) r--;

                        evaluation = -AlphaBeta<OColor, false, false>(
                            ply + 1,
                            std::max<int16_t>(depth - r, 1),
                            -alpha - 1,
                            -alpha
                        );
                    } else
                        evaluation = alpha + 1;

                    if (evaluation > alpha) {
                        evaluation = -AlphaBeta<OColor, false, false>(ply + 1, depth - 1, -alpha - 1, -alpha);

                        if (evaluation > alpha && evaluation < beta)
                            evaluation = -AlphaBeta<OColor, true, false>(ply + 1, depth - 1, -beta, -alpha);
                    }
                }

                UndoMove<true>(state, move);

                if (evaluation <= bestEvaluation) continue;

                bestEvaluation = evaluation;

                if (evaluation <= alpha) continue;

                alpha = evaluation;

                ttEntryNew.Type = Exact;
                ttEntryNew.Move =  move;

                if (ThreadType == Main && PV && Status != SearchThreadStatus::Stopped) {
                    PVTable[ply].PV[ply] = move;

                    for (uint8_t nthPly = ply + 1; nthPly < PVTable[ply + 1].Ply; nthPly++)
                        PVTable[ply].PV[nthPly] = PVTable[ply + 1].PV[nthPly];

                    PVTable[ply].Ply = PVTable[ply + 1].Ply;
                }

                if (evaluation < beta) continue;

                if (Status != SearchThreadStatus::Stopped && quiet) {
                    if (Killer[0][ply] != move) {
                        Killer[1][ply] = Killer[0][ply];
                        Killer[0][ply] = move;
                    }

                    UpdateHistory<Color, true>(move, depth);

                    for (uint8_t q = 1; q < quietMoves; q++)
                        UpdateHistory<Color, false>(moves.UnsortedAccess(i - q), depth);
                }

                ttEntryNew.Type = Beta;
                break;
            }

            ttEntryNew.Evaluation = CompressScore(bestEvaluation);

            if (Status != SearchThreadStatus::Stopped) TryReplaceTTEntry(ttEntry, ttEntryNew);

            return bestEvaluation;
        }

        template<Color Color, bool PV>
        Score Quiescence(const uint8_t ply, Score alpha, const Score beta)
        {
            constexpr auto OColor = Opposite(Color);

            if (ThreadType == Main && PV) SelectiveDepth = std::max(SelectiveDepth, ply);

            if (!PV) {
                const ZobristHash hash = Board.Zobrist();

                const SearchTranspositionEntry& ttEntry = TT[hash];

                if (ttEntry.Hash == CompressHash(hash)) {
                    if (ttEntry.Type == Exact                               ) return ttEntry.Evaluation;
                    if (ttEntry.Type == Beta  && ttEntry.Evaluation >= beta ) return ttEntry.Evaluation;
                    if (ttEntry.Type == Alpha && ttEntry.Evaluation <= alpha) return ttEntry.Evaluation;
                }
            }

            const Score staticEvaluation = Evaluation::Evaluate(Color, ThreadId);

            if (staticEvaluation >= beta) return beta;
            if (staticEvaluation > alpha) alpha = staticEvaluation;

            using MoveList = OrderedMoveList<Color, true>;

            MoveList moves (Board, ply, Killer, History);

            Score bestEvaluation = staticEvaluation;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move = moves[i];

                if (!SEE::Accurate(Board, move, 0)) continue;

                const PreviousState state = DoMove<false>(move, ply);

                const Score evaluation = -Quiescence<OColor, PV>(ply + 1, -beta, -alpha);

                UndoMove<false>(state, move);

                if (evaluation <= bestEvaluation) continue;

                bestEvaluation = evaluation;

                if (evaluation <= alpha) continue;

                alpha = evaluation;

                if (evaluation >= beta) break;
            }

            return bestEvaluation;
        }

        template<bool UpdateRepetitionHistory>
        PreviousState DoMove(const Move move, const uint8_t ply, const bool quiet = false)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            if (!quiet || Board[move.From()].Piece() == Pawn) {
                Stack[ply + 1].HalfMoveCounter = 1;
            } else
                Stack[ply + 1].HalfMoveCounter = Stack[ply].HalfMoveCounter + 1;

            const PreviousState state = Board.Move<MT>(move.From(), move.To(), move.Promotion(), ThreadId);
            Nodes++;

            const ZobristHash hash = Board.Zobrist();

            TT.Prefetch(hash);

            if (UpdateRepetitionHistory) Repetition.Push(hash);

            return state;
        }

        template<bool UpdateRepetitionHistory>
        void UndoMove(const PreviousState state, const Move move)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            Board.UndoMove<MT>(state, move.From(), move.To(), ThreadId);

            if (UpdateRepetitionHistory) Repetition.Pop();
        }

        template<Color Color, bool Increase>
        void UpdateHistory(const Move move, const int16_t depth)
        {
            const int16_t bonus = std::min<int16_t>(300 * depth - 250, HistoryLimit);

            int16_t& history = History[Color][Board[move.From()].Piece()][move.To()];

            history += bonus * (Increase ? 1 : -1) - history * bonus / HistoryLimit;
        }

        static void TryReplaceTTEntry(SearchTranspositionEntry& pEntry, const SearchTranspositionEntry nEntry)
        {
            if (nEntry.Type == Exact || nEntry.Hash != pEntry.Hash ||
               (pEntry.Type == Alpha &&
                nEntry.Type == Beta) ||
                nEntry.Depth > pEntry.Depth - TTReplacementDepthMargin)
                pEntry = nEntry;
        }

    };

    class ParallelTaskPool
    {

        using ParallelTask = SearchTask<Parallel>;

        std::vector<ParallelTask> Internal;

        size_t TaskCount = 0;

        public:
        ParallelTaskPool()
        {
            Clear ();
            Resize();
        }

        void Resize()
        {
            TaskCount = ThreadPool.Size() - 1;

            Internal.reserve(TaskCount);
        }

        void Clear() { Internal.clear(); }

        size_t Size() const { return TaskCount; }

        void Fill(Limit& l, Board& b, RepetitionStack& r, const uint8_t hmc)
        { for (size_t i = 0; i < TaskCount; i++) Internal.emplace_back(l, b, r, hmc, i + 1); }

        constexpr std::vector<ParallelTask>& operator &(){ return Internal; }

    };

    template<typename MainEventHandler = DefaultSearchEventHandler>
    struct ThreadedSearch
    {

        static inline ParallelTaskPool ParallelTaskPool;

        struct ThreadedSearchTaskPassthroughHandler : DefaultSearchEventHandler
        {

            static void HandleIterativeDeepeningIterationCompletion(const IterativeDeepeningIterationCompletionEvent& e)
            {
                IterativeDeepeningIterationCompletionEvent event = e;

                for (const auto& task : &ParallelTaskPool) event.Nodes += task.GetNodes();

                return MainEventHandler::HandleIterativeDeepeningIterationCompletion(event);
            }

            static void HandleIterativeDeepeningCompletion(const IterativeDeepeningCompletionEvent& e)
            { MainEventHandler::HandleIterativeDeepeningCompletion(e); }

        };

        using MainSearchTask = SearchTask<Main, ThreadedSearchTaskPassthroughHandler>;

        static inline MainSearchTask MainTask;

        static inline bool Searching = false;

        static void Run(Limit& l, Board& b, RepetitionStack& r, const uint8_t hmc)
        {
            if (Searching) return;

            Searching = true;

            if (ParallelTaskPool.Size()) {
                ParallelTaskPool.Fill(l, b, r, hmc);

                for (auto& task : &ParallelTaskPool) ThreadPool.Execute(
                    [&task] -> void
                    {
                        task.IterativeDeepening();
                    }
                );
            }

            MainTask = MainSearchTask(l, b, r, hmc, 0);

            ThreadPool.Execute(
                [] -> void
                {
                    MainTask.IterativeDeepening();

                    if (ParallelTaskPool.Size()) {
                        for (auto& task : &ParallelTaskPool) task.Stop();

                        for (auto& task : &ParallelTaskPool) if (!task.Stopped()) Sleep(1);
                    }

                    ParallelTaskPool.Clear();
                    Searching = false;
                }
            );
        }

    };

} // StockDory

#endif //STOCKDORY_SEARCH_H
