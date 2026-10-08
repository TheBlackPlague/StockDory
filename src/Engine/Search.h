//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_SEARCH_H
#define STOCKDORY_SEARCH_H

#include <algorithm>
#include <cmath>
#include <memory>
#include <ranges>

#include "../Backend/Board.h"
#include "../Backend/Misc.h"
#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Move.h"

#include "Common.h"
#include "OrderedMoveList.h"
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

    CompressedHash  CompressHash (const ZobristHash hash) { return hash; }

    CompressedScore CompressScore(const Score score, const uint8_t ply)
    {
        return IsWin(score) ? score + ply : IsLoss(score) ? score - ply : score;
    }

    Score DecompressScore(const CompressedScore score, const uint8_t ply)
    {
        return IsWin(score) ? score - ply : IsLoss(score) ? score + ply : score;
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

    inline auto LMRTable =
    [] -> Array<int32_t, MaxDepth, MaxMove>
    {
        const auto formula = [](const size_t depth, const size_t move) -> int32_t
        {
            const int32_t value = (std::log(depth) * std::log(move) / 2 - 0.2) * LMRQuantization;

            return value / LMRQuantization > 0 ? value : 0;
        };

        Array<int32_t, MaxDepth, MaxMove> temp {};

        for (size_t depth = 1; depth < MaxDepth; depth++)
        for (size_t move  = 1; move  < MaxMove ;  move++) temp[depth][move] = formula(depth, move);

        return temp;
    }();

    class SearchStack
    {

        public:
        struct Frame
        {

            Score   StaticEvaluation = None;
            uint8_t HalfMoveCounter  =    0;

            Piece PieceToMove = NAP;
            Move         Move = { };

        };

        private:
        constexpr static size_t Padding = 8;

        Array<Frame, Padding + MaxDepth + 1> Internal {};

        public:
              Frame& operator [](const size_t index)       { return Internal[index + Padding]; }
        const Frame& operator [](const size_t index) const { return Internal[index + Padding]; }

    };

    class SearchedMovesStack
    {

        Array<Move, MaxMove> Internal;

        uint8_t Count = 0;

        public:
        void Push(const Move move) { Internal[Count++] = move; }

        uint8_t Size() const { return Count; }

        auto begin() const { return Internal.begin()        ; }
        auto   end() const { return Internal.begin() + Count; }

    };

    constexpr size_t RepetitionLimit = 3;

    class RepetitionStack
    {

        Array<ZobristHash, 4096> Internal {};

        size_t CurrentIndex = 0;
        size_t NullBoundary = 0;

        public:
        void Push(const ZobristHash hash) { Internal[CurrentIndex++] = hash; }

        void Pop() { CurrentIndex--; }

        size_t PushNull(const ZobristHash hash)
        {
            const size_t previous = NullBoundary;

            NullBoundary = CurrentIndex;

            Push(hash);

            return previous;
        }

        void PopNull(const size_t previous) { Pop(); NullBoundary = previous; }

        bool Found(const uint8_t halfMoveCounter) const
        {
            if (CurrentIndex <= NullBoundary) return false;

            const size_t current = CurrentIndex - 1;
            const size_t limit   = std::min<size_t>(halfMoveCounter, current - NullBoundary);

            const ZobristHash hash = Internal[current];

            uint8_t found = 1;

            for (size_t distance = 4; distance <= limit; distance += 2) {
                if (Internal[current - distance] != hash) continue;

                if (++found == RepetitionLimit) return true;
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

    using PVTable = Array<PVEntry, MaxDepth + 1>;

    enum SearchTaskStatus : uint8_t
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

        enum TimeType : uint8_t { Actual, Optimal };

        uint64_t Nodes = std::numeric_limits<uint64_t>::max();
        uint8_t  Depth = MaxDepth - 1;

        bool Timed = false;
        bool Fixed = false;

        MS  ActualTime {};
        MS    BaseTime {};
        MS OptimalTime {};

    };

    class WDLCalculator
    {

        constexpr static double Scale = 1000.0;

        struct Coefficient { double A = 0.0; double B = 0.0; };

        enum Weight : uint8_t { A, B };

        constexpr static Array<double, 2, 4> WValue = {{
            { -39.30487954, 262.98923259, -419.98251081, 445.64485738 },
            { -19.92536318, 135.60358857, -123.93301877, 103.36733479 }
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

            const Score material = std::clamp<Score>(
                Count(pawn  ) * 1 +
                Count(knight) * 3 +
                Count(bishop) * 3 +
                Count(rook  ) * 5 +
                Count(queen ) * 9 ,
                17,
                78
            );

            return { .A = Formula<A>(material), .B = Formula<B>(material) };
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

        uint8_t          Depth {};
        uint8_t SelectiveDepth {};

        Score Evaluation {};
        WDL          WDL {};

        uint64_t Nodes {};
        MS        Time {};

        PVEntry PVEntry {};

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
    class alignas(CacheLineSize) SearchTask
    {

        Board Board {};

        KTable Killer  {};
        HTable History {};

        CaptureHTable CaptureHistory {};

         CHTable           ContinuationHistory {};
        CCHTable CorrectionContinuationHistory {};

        MinorCTable MinorCorrectionHistory {};
        MajorCTable MajorCorrectionHistory {};

        SearchStack Stack {};

        RepetitionStack Repetition {};

        PVTable PVTable {};

        Limit Limit {};

        uint8_t          Depth = 0;
        uint8_t SelectiveDepth = 0;

        Score Evaluation = -Infinity;

        Move BestMove {};

        TP StartTime = {};

        uint64_t     RootNodes = 0;
        uint64_t BestMoveNodes = 0;

        Move LastBestMove {};

        uint8_t BestMoveStability = 0;

        bool SingleMove = false;

        size_t ThreadId = 0;

        Atomic<    uint64_t    > Nodes  {    0    };
        Atomic<SearchTaskStatus> Status { Running };

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

        uint64_t GetNodes() const { return Nodes.Load(MemoryOrder::relaxed); }

        void IterativeDeepening()
        {
            if (ThreadType == Main) {
                // If we are in the main thread, we should see if we have a single move in this position. If that's the
                // case, we can save time by searching far less (ideally just a few depths) - saving time for when we
                // have more choices
                SearchSingleMoveTimeOptimization();

                // Set the starting point for all time measurements
                StartTime = std::chrono::steady_clock::now();
            }

            // Load the board state for evaluation purposes (this needs to be done for all threads with their respective
            // thread IDs as each thread has its own evaluation state)
            Board.LoadForEvaluation(ThreadId);

            Depth = 1;
            while (Depth < MaxDepth && Depth <= Limit.Depth && !OutOfTime<Limit::Optimal>()) {
                if (Board.ColorToMove() ==   White)
                     Evaluation = Aspiration<White>(Depth);
                else Evaluation = Aspiration<Black>(Depth);

                // In the case that the search was stopped, we should just proceed to fire the completion event
                if (Stopped()) break;

                if (ThreadType == Main) {
                    // On the main thread, we need to fire events to notify handlers about the completion of the
                    // iterative deepening iteration and provide them with the results. Furthermore, if needed, the
                    // main thread is also responsible for optimizing the time allocation for the next iterations

                    const auto time = ElapsedTime();

                    SearchTimeManagement();

                    EventHandler::HandleIterativeDeepeningIterationCompletion({
                        .         Depth =          Depth,
                        .SelectiveDepth = SelectiveDepth,

                        .Evaluation = WDLCalculator::S(Board, Evaluation),
                        .WDL        = WDL             (Board, Evaluation),

                        .Nodes = GetNodes(),
                        .Time  = time,

                        .PVEntry = PVTable[0]
                    });
                }

                Depth++;
            }

            Stop();

            if (ThreadType == Main) {
                // The main thread is responsible for notifying the handlers about the completion of the search,
                // providing them with the best move found

                EventHandler::HandleIterativeDeepeningCompletion({
                    .Move = BestMove
                });
            }
        }

        void Stop() { Status.Store(SearchTaskStatus::Stopped, MemoryOrder::relaxed); }

        bool Stopped() const { return Status.Load(MemoryOrder::relaxed) == SearchTaskStatus::Stopped; }

        Score GetEvaluation() const { return WDLCalculator::S(Board, Evaluation); }

        WDL GetWDL() const { return WDL(Board, Evaluation); }

        MS ElapsedTime() const
        { return std::chrono::duration_cast<MS>(std::chrono::steady_clock::now() - StartTime); }

        private:
        void IncrementNodes() { Nodes.Store(Nodes.Load(MemoryOrder::relaxed) + 1, MemoryOrder::relaxed); }

        template<Limit::TimeType Type>
        bool OutOfTime() const
        {
            if (ThreadType != Main) return false;

            if (!Limit.Timed) return false;

            return Type == Limit::Actual ? ElapsedTime() > Limit. ActualTime
                                         : ElapsedTime() > Limit.OptimalTime;
        }

        void SearchSingleMoveTimeOptimization()
        {
            if (!Limit.Timed) return;
            if ( Limit.Fixed) return;

            uint8_t moveCount;

            if (Board.ColorToMove() == White) {
                const OrderedMoveList<White, false, false> moves (Board);
                moveCount = moves.Count();
            } else {
                const OrderedMoveList<Black, false, false> moves (Board);
                moveCount = moves.Count();
            }

            SingleMove = moveCount <= 1;

            if (!SingleMove) return;

            Limit.OptimalTime = MS(
                Limit.BaseTime.count() * TimeSingleMovePartitionNumerator / TimeSingleMovePartitionDenominator
            );
            Limit.ActualTime = Limit.BaseTime;
        }

        void SearchTimeManagement()
        {
            if (!Limit.Timed) return;
            if ( Limit.Fixed) return;

            if (SingleMove) return;

            if (LastBestMove && LastBestMove == BestMove)
                 BestMoveStability = std::min<uint8_t>(BestMoveStability + 1, TimeBestMoveStabilityMax);
            else BestMoveStability = 0;

            LastBestMove = BestMove;

            if (Depth < TimeManagementMinimumDepth) return;

            double factor = 1.0;

            if (RootNodes > 0) {
                const double effort = static_cast<double>(BestMoveNodes) / RootNodes;
                factor *= TimeNodeBase - TimeNodeEffortWeight * effort;
            }

            factor *= 1.0 + (1.0 - static_cast<double>(BestMoveStability) / TimeBestMoveStabilityMax   ) *
                                                                            TimeBestMoveStabilityWeight  ;

            const double scaledTime = static_cast<double>(Limit.BaseTime.count()) * factor;

            const uint64_t optimalTime = static_cast<uint64_t>(
                std::min(scaledTime, static_cast<double>(Limit.ActualTime.count()))
            );

            Limit.OptimalTime = MS(optimalTime);
        }

        template<Color Color>
        Score Aspiration(const int16_t depth)
        {
            Score alpha = -Infinity;
            Score beta  =  Infinity;

            // Window Configuration:
            //
            // If we have done enough full window search iterations at lower depths to get a relatively accurate
            // evaluation, then all future search iterations can be done with a smaller window centered around the
            // evaluation from the previous search iteration: (evaluation - margin, evaluation + margin)
            if (depth >= AspirationWindowMinimumDepth) {
                alpha = Evaluation - AspirationWindowMargin;
                beta  = Evaluation + AspirationWindowMargin;
            }

            uint8_t research = 0;
            while (true) {
                if (ThreadType == Main) {
                    // If we are in the main thread, we should regularly (every search/research) check if the search's
                    // limits have been crossed. If they have, we should stop searching/researching
                    if (OutOfTime<Limit::Actual>()) [[unlikely]]
                        Stop();
                }

                // If the search was stopped, we should return a draw score immediately
                if (Stopped()) [[unlikely]] return Draw;

                // Window Fallback:
                //
                // If previous search and researches have failed to find a move within the window, then our window is
                // likely not capturing the relevant part of the search space. In this case, we should reset to a full
                // window and try again for future search iterations with a better understanding of the search space
                if (alpha < -AspirationWindowFallbackBound) alpha = -Infinity;
                if (beta  >  AspirationWindowFallbackBound) beta  =  Infinity;

                if (ThreadType == Main) {
                    RootNodes     = 0;
                    BestMoveNodes = 0;
                }

                const Score bestEvaluation = PVS<Color, true, true>(0, depth, alpha, beta);

                // Possible Search Window Extending:
                //
                // The search window is centered around the evaluation, but it may not always capture initially capture
                // the relevant part of the search space. This can happen if the evaluation from the previous search
                // iterations were not accurate or representative enough or if our initial margin was too small. In this
                // case, we should extend the search window and research the position to find a more accurate
                // evaluation. The window is extended at a quadratic rate to ensure that we can avoid researches and
                // quickly converge to capture the relevant part of the search space:
                //
                // - fail-low : lower bound (alpha) is too high, we need to decrease it
                // - fail-high: upper bound (beta ) is too low , we need to increase it
                //
                // If the window is capturing the relevant part of the search space, then we can return the best
                // evaluation found so far

                if        (bestEvaluation <= alpha) {
                    research++;

                    alpha = std::max<Score>(alpha - research * research * AspirationWindowMarginDelta, -Infinity);
                } else if (bestEvaluation >= beta) {
                    research++;

                    beta  = std::min<Score>(beta  + research * research * AspirationWindowMarginDelta,  Infinity);

                    BestMove = PVTable[0].PV[0];
                } else {
                    BestMove = PVTable[0].PV[0];
                    return bestEvaluation;
                }
            }
        }

        template<Color Color, bool PV, bool Root, bool NMPAllowed = true>
        Score PVS(const uint8_t ply, int16_t depth, Score alpha, Score beta)
        {
            // Opponent's color for recursive calls
            constexpr auto OColor = Opposite(Color);

            if (ThreadType == Main) {
                const uint64_t nodes = GetNodes();

                // If we are in the main thread, we should regularly (every 4096 nodes) check if the search's limits
                // have been crossed. If they have, we should stop searching
                if ((nodes & 4095) == 0 && OutOfTime<Limit::Actual>()) [[unlikely]]
                    Stop();

                // If we have exceeded the maximum node limit, we should stop searching
                if (nodes > Limit.Nodes) [[unlikely]]
                    Stop();
            }

            // If the search was stopped, we should return a draw score immediately
            if (Stopped()) [[unlikely]] return Draw;

            if (ThreadType == Main) {
                // The main thread is responsible for ensuring the PV Table is correctly updated with the right moves
                // and that the correct selective depth is reported

                PVTable[ply].Ply = ply;

                if (PV) SelectiveDepth = std::max(SelectiveDepth, ply);
            }

            const bool checked = Board.Checked<Color>();

            // We should stop searching if we have reached the maximum depth as otherwise we may start accessing
            // memory we haven't allocated
            if (ply >= MaxDepth) [[unlikely]]
                return checked ? Draw : CorrectEvaluation<Color>(ScaleEvaluation<Color>(), ply);

            const ZobristHash hash = Board.Zobrist();

            if (!Root) {
                // If we are not at the root of the search, we should check if the position we've reached is a draw. We
                // cannot accurately determine if any position is drawn without a full search till the end. However, we
                // can do simple checks to see if the position is drawn by the 50-move rule, repetition, of if there is
                // not enough material for either side to win

                // 50-move rule:
                //
                // The 50-move rule states that if there have been 50 full moves without a pawn move or a capture, then
                // the game is drawn. The half-move counter tracks the number of half-moves since the last pawn move or
                // capture, so if it reaches 100, the game is drawn
                if (Stack[ply].HalfMoveCounter >= 100) {
                    if (!checked) return Draw;

                    const OrderedMoveList<Color, false, false> moves (Board);

                    // Out of Moves:
                    //
                    // Checkmate takes precedence over the 50-move rule
                    return moves.Count() == 0 ? LossIn(ply) : Draw;
                }

                // Repetition:
                //
                // The repetition rule states that if the same position occurs three times, then the game is drawn. We
                // check this by storing the Zobrist Hashes of all positions we've seen in the current branch of the
                // search. We check if the current position's hash has been seen before N times, where N is equal to
                // the repetition limit (3 by default)
                if (Repetition.Found(Stack[ply].HalfMoveCounter)) return Draw;

                // Insufficient material:
                //
                // If there are not enough pieces on the board at the right squares to win, the game is drawn. This can
                // become a bit tricky, as there are many cases where this can happen. We check for the simplest cases:
                // - If there are only kings left
                // - If there are only kings and a knight left (belonging to either side)
                // - If there are only kings and a bishop left (belonging to either side)
                {
                    const uint8_t pieceCount = Count(~Board[NAC]);

                    if (pieceCount == 2) return Draw;

                    const bool knightLeft = Board.PieceBoard<White>(Knight) | Board.PieceBoard<Black>(Knight),
                               bishopLeft = Board.PieceBoard<White>(Bishop) | Board.PieceBoard<Black>(Bishop);

                    if (pieceCount == 3 && (knightLeft || bishopLeft)) return Draw;
                }

                // Mate Distance Pruning:
                //
                // Assuming we may be in a position where we'll mate the opponent in the next ply, at best our
                // evaluation will be Mate - ply - 1. If our alpha is greater or equal to this evaluation, then we know
                // that an equally short or shorter mate was already found at some previous ply, and thus there is not
                // much point in continuing further since we will not find a shorter mate in this branch
                alpha = std::max<Score>(alpha, LossIn(ply    ));
                beta  = std::min<Score>(beta ,  WinIn(ply + 1));
                if (alpha >= beta) return alpha;
            }

            depth = std::min<int16_t>(depth, MaxDepth - 1);

            // If we've exhausted our search depth and aren't in check, we should check if there are any tactical
            // sequences just over the horizon. If there are, we should get a more accurate evaluation through a
            // Quiescence search. If we are in check, we should go down the normal search path, extending as needed to
            // ensure we find a suitable evasion
            if (depth <= 0 && !checked) return Quiescence<Color, PV>(ply, alpha, beta);

            // Transposition Table Reading:
            //
            // We can check if the current position has been searched before, and if it has, then there most likely
            // exists a transposition entry - if the entry is valid, depending on the quality of the entry, we can
            // return the evaluation from the entry. Even if the entry isn't of sufficient quality to return directly,
            // we can still search the move in the entry first, since it most likely is the best move in the position
            SearchTranspositionEntry ttEntry      = TT[hash];
            Move                     ttMove       = {};
            bool                     ttHit        = false;
            Score                    ttEvaluation = None;

            if (ttEntry.Type != Invalid && ttEntry.Hash == CompressHash(hash)) {
                ttHit  = true;
                ttMove = ttEntry.Move;

                ttEvaluation = DecompressScore(ttEntry.Evaluation, ply);

                if (!PV && ttEntry.Depth >= depth) {
                    // If the entry is of sufficient quality, depending on the bounding type of the entry, we can
                    // directly return the evaluation from the entry. We shouldn't do this in PV branches as even a
                    // slight inaccuracy due to hash collisions or other factors can cause us to miss a good move.
                    //
                    // An entry's quality is currently determined by:
                    // - entry depth >= current search depth
                    //
                    // Returning the evaluation from the entry if the bounding type is:
                    // - Exact: The evaluation is accurate and representative of an actual search
                    // - Beta : The evaluation caused a beta cut-off in the producing search, but we should only return
                    //          if it is capable of causing a beta cut-off in the current search
                    // - Alpha: The evaluation never exceeded alpha in the producing search, but we should only return
                    //          if we know it isn't exceeding alpha in the current search

                    if (ttEntry.Type == Exact                         ) return ttEvaluation;
                    if (ttEntry.Type == Beta  && ttEvaluation >= beta ) return ttEvaluation;
                    if (ttEntry.Type == Alpha && ttEvaluation <= alpha) return ttEvaluation;
                }
            }

            const bool majorMaterial = Board.HasMajorMaterial<Color>();

            // Internal Iterative Reduction (IIR):
            //
            // If we are at a high enough depth but there is no valid transposition table move, we can reduce the search
            // depth by a small amount to speed up the search
            if (depth >= IIRMinimumDepth && !ttMove)
                depth -= IIRDepthReduction;

            Score scaledEvaluation;
            Score staticEvaluation;
            bool  improving;

            if (checked) {
                // Last non-checked Static Evaluation:
                //
                // If the position is under check, static evaluation is most likely not reliable. As such, we should
                // use the static evaluation from the last ply where we were not checked, which hopefully is our last
                // turn, but may also be the turn before that if we were checked on the last turn.
                //
                // Static evaluation of the current position is set to the static evaluation two plies ago, which, if we
                // were in check, recursively, is the static evaluation from four plies ago, and so on. This essentially
                // means the static evaluation from two plies ago is the static evaluation from the last ply where we
                // were not checked which could be two plies ago, four plies ago, or any N * 2 plies ago
                staticEvaluation = Stack[ply].StaticEvaluation = Stack[ply - 2].StaticEvaluation;

                // Positionally Improving:
                //
                // Typically, getting in check is a sign that we aren't positionally improving
                improving = false;

                // Check Extension:
                //
                // If we are under check, we should search this branch deeper since we need to find a good way to evade
                // the check
                depth = std::min<int16_t>(depth + CheckExtension, MaxDepth - 1);

                // Avoid Risky Pruning:
                //
                // If we are under check, we should avoid risky pruning techniques that could otherwise prevent us from
                // finding a good move to evade this check, in turn, preventing us from getting to a better position. We
                // would then be forced to consider this branch a loss, which may not be the case if we were to evade
                // the check properly and get to a better position
                goto Checked;
            }

            // Static Evaluation:
            //
            // If we're not in check, then we are more-so safe to use the static evaluation to guide some aggressive
            // pruning & reduction techniques in an attempt to narrow the search space. We calculate it by applying
            // material scaling to the neural network's evaluation of the position and then adjusting it using the
            // search-trained correction history; it is important to retain the evaluation pre-adjustment so we can
            // later use it for error criterion to train the correction history.
            //
            // A valid transposition table entry can then replace or refine this static evaluation:
            //
            // - Exact:
            //   We can use the search result directly as it will almost always be just as accurate (if not more) then
            //   whatever our static evaluation is.
            //
            // - Beta :
            // - Alpha:
            //   In this case, the type indicates the position is either too good or too unsalvageable. Thus we will go
            //   one step further in the direction of the extremity and respectively use either the more optimistic or
            //   pessimistic evaluation.
            //
            // Without a valid entry, the corrected evaluation should be used; correction history is never applied to
            // the evaluation the transposition table
            if (ttHit) {
                staticEvaluation = ttEvaluation;

                if (ttEntry.Type != Exact) {
                    scaledEvaluation =   ScaleEvaluation<Color>(                     );
                    staticEvaluation = CorrectEvaluation<Color>(scaledEvaluation, ply);

                    if      (ttEntry.Type == Beta ) staticEvaluation = std::max<Score>(staticEvaluation, ttEvaluation);
                    else if (ttEntry.Type == Alpha) staticEvaluation = std::min<Score>(staticEvaluation, ttEvaluation);
                }
            } else {
                scaledEvaluation =   ScaleEvaluation<Color>(                     );
                staticEvaluation = CorrectEvaluation<Color>(scaledEvaluation, ply);
            }

            Stack[ply].StaticEvaluation = staticEvaluation;

            // Positionally Improving:
            //
            // Check to see if we are improving positionally. This is done by comparing the current position's static
            // evaluation to the static evaluation of the position last time we were not under check.
            //
            // The static evaluation of the position two plies ago will either be the static evaluation from two plies
            // ago if we were not under check, or the static evaluation from N * 2 plies ago if we were under check.
            // This is explained more in detail in the "Last non-checked Static Evaluation" comment above
            improving = staticEvaluation > Stack[ply - 2].StaticEvaluation;

            if (!PV) {
                // Risky Pruning:
                //
                // The techniques below are risky pruning techniques that can cause us to miss some good moves. Doing
                // this in PV branches can be disastrous, however, in non-PV branches, this is relatively safe to do. We
                // can afford to miss some good moves in non-PV branches, as we are not that likely going to find the
                // best move in these branches, mainly using the results of these branches to optimize search tree
                // exploration

                // Reverse Futility Pruning (RFP):
                //
                // RFP is a pruning technique that allows us to prune branches that are too good for us, meaning that
                // they are inversely too bad for the opponent; the opponent will likely avoid these branches entirely,
                // so it's not worth our time to search them. This is done by comparing the static evaluation with the
                // upper bound of the search (beta) and if the static evaluation is significantly better, then it is
                // likely below the lower bound of the search (alpha) for the opponent. The word "likely" is used here
                // as static evaluation is in most cases just an approximation of an actual search, so we cannot say
                // for sure. Taking the average between the static evaluation and the upper bound of the search (beta)
                // allows us to get a more representative evaluation of the position that isn't too optimistic or one
                // that underestimates the position
                if (depth < ReverseFutilityMaximumDepth && abs(beta) < Mate) {
                    const Score     depthMargin = depth     * ReverseFutilityDepthFactor;
                    const Score improvingMargin = improving * ReverseFutilityImprovingFactor;

                    if (staticEvaluation - depthMargin + improvingMargin >= beta) return (staticEvaluation + beta) / 2;
                }

                // Razoring:
                //
                // Razoring is a pruning technique that allows us to prune branches that are too bad for us to be worth
                // fully exploring. There may be some sequences that can let us recover from the current position, but
                // these are rare and more often than not, they are tactical sequences. Non-tactical sequences that can
                // recover from the current position are so rare that given we are in a non-PV branch, we can afford to
                // miss them. As such, we can step through the tactical sequences (if any are available) in Quiescence
                // search and return the resulting evaluation
                if (depth == RazoringDepth && staticEvaluation + RazoringEvaluationMargin < alpha)
                    return Quiescence<Color, false>(ply, alpha, beta);

                // Null Move Pruning (NMP):
                //
                // NMP is a pruning technique that allows us to prune branches that are too good for us (like RFP), but
                // unlike RFP, we use a different approach. Instead of relying on static evaluation, we instead rely on
                // the conceptual understanding that doing nothing (i.e., not moving) is most likely worse than making
                // a move. If we are in a position where we can skip a move (skip our turn) and still be in a position
                // where we produce a beta cut-off, then most likely this branch is too good for us. Our opponent will
                // likely avoid this branch entirely, so it's not worth our time searching it further. We check this by
                // making a null move (skipping our turn) and then searching the position at a reduced depth with a
                // binary search window, centered around our upper bound (beta) as their lower bound (alpha). If the
                // branch is bad for the opponent, they'll be unable to improve upon their lower bound and fail-low. In
                // turn, this can allow us to produce a beta cut-off and prune this branch
                if (!Root && NMPAllowed && depth >= NullMoveMinimumDepth && staticEvaluation >= beta &&
                    majorMaterial && !IsMate(beta) && !IsMate(staticEvaluation)) {
                    // The reduced depth is determined by the below formula:
                    //
                    // d = current depth
                    // r = reduced depth
                    // e = static evaluation
                    // b = beta
                    // dF = NullMoveDepthFactor
                    // eF = NullMoveEvaluationFactor
                    // mR = NullMoveMinimumReduction
                    //
                    // Using "//" to denote integer division - in C++, integer division approaches towards zero
                    //
                    // r = d - (mR + d // dF + min((e - b) // eF, mR))

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

                    Stack[ply].PieceToMove = NAP;
                    Stack[ply].       Move = { };

                    Stack[ply + 1].HalfMoveCounter = Stack[ply].HalfMoveCounter;

                    const PreviousStateNull state = Board.Move();

                    const size_t previous = Repetition.PushNull(Board.Zobrist());

                    const auto evaluation = -PVS<OColor, false, false, false>(
                        ply + 1,
                        reducedDepth,
                        -beta,
                        -beta + 1
                    );

                    Repetition.PopNull(previous);

                    Board.UndoMove(state);

                    if (evaluation >= beta) return beta;
                }
            }

            // We continue from here if we are in check
            Checked:

            using MoveList = OrderedMoveList<Color>;

            const HTable& continuation = Stack[ply - 1].Move ?
                ContinuationHistory[Stack[ply - 1].PieceToMove][Stack[ply - 1].Move.To()] : NullHistory;

            MoveList moves (Board, ply, Killer, History, continuation, CaptureHistory, ttMove);

            // Out of Moves:
            //
            // If we have no moves to search at this point, it is either because we are in checkmate or stalemate
            if (moves.Count() == 0) return checked ? LossIn(ply) : Draw;

            SearchTranspositionEntry ttEntryNew
            {
                .Hash       = CompressHash(hash),
                .Move       = ttMove,
                .Depth      = static_cast<uint8_t>(depth),
                .Type       = Alpha
            };

            SearchedMovesStack searchedCaptures;
            SearchedMovesStack searchedQuiets  ;

            uint8_t encounteredQuiets = 0;

            Score bestEvaluation = -Infinity;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move = moves[i];

                const bool capture = move.Capture();
                const bool quiet   = move.Quiet  ();

                encounteredQuiets += quiet;

                if (!checked && quiet) {
                    if (i >= 1) {
                        const Score margin = depth * FutilityDepthFactor;

                        if (staticEvaluation + margin <= alpha) break;
                    }

                    if (!Root && !PV && majorMaterial && depth <= LMPMaximumDepth &&
                        encounteredQuiets > LMPLastQuietBase + depth * depth &&
                        bestEvaluation > -Infinity)
                        break;
                }

                if (!Root && !checked && depth <= SEEMaximumDepth && !IsLoss(bestEvaluation)) {
                    const Score margin = quiet ? SEEQuietDepthFactor * depth : SEECaptureDepthFactor * depth * depth;

                    if (!SEE::Accurate(Board, move, -margin)) continue;
                }

                const Piece movingPiece = Board[move.From()].Piece();

                uint64_t nodesBeforeMove = 0;

                if (Root && ThreadType == Main) nodesBeforeMove = GetNodes();

                const PreviousState state = DoMove<true>(move, ply);

                // Principle Variation Search (PVS):
                //
                // PVS is a search technique that heavily relies on the move policy. It works on the assumption that the
                // move policy ensures that the best moves are ordered first, and thus, the first move is likely the
                // best move. As such, the first move is searched with a full window (alpha, beta), while all future
                // moves are searched with initially with a reduced window (alpha - 1, alpha). If the reduced window
                // search produces favorable results (i.e., the evaluation is greater than alpha), then we research
                // with a full window (alpha, beta) to properly evaluate the move. Due to the transposition table, all
                // researches are relatively inexpensive, and the time we save ignoring moves that don't have potential
                // to improve our position more than the previous moves is worth it.

                Score evaluation = 0;

                if (i == 0) evaluation = -PVS<OColor, PV, false>(ply + 1, depth - 1, -beta, -alpha);
                else {
                    // Assume we are not in a PV branch and use a reduced window search. If the reduced window search
                    // shows potential to improve our position, we will research with a full window search assuming
                    // we are in a PV branch

                    // Late Move Reduction and Extension (LMR-E):
                    //
                    // Base LMR is a reduction technique that allows us to reduce the depth of the search for moves that
                    // appear later since, according to the move policy, they are likely worse than the moves that were
                    // searched earlier. However, LMR-E allows us to extend the search depth for moves that may be very
                    // tactically promising or likely to fail high (i.e., produce a beta cut-off). If the reduced or
                    // extended search gives a promising evaluation (i.e., greater than our current lower bound, which
                    // is alpha), we then research them at a full depth. The researches are relatively inexpensive due
                    // to the transposition table, and the time we save by not searching moves that are unlikely to
                    // improve our position is worth it
                    if (!checked && depth >= LMRMinimumDepth && i >= LMRMinimumMoves) {
                        // Reduction values are determined by a formula that takes into account the current depth and
                        // move number. Current formula:
                        //
                        // r = floor((ln(depth) * ln(i) / 2 - 0.2) * LMRGranularityFactor)
                        int32_t r = LMRTable[depth][i];

                        // If we are not in a PV branch, we can afford to reduce the search depth further
                        if (!PV) r += LMRNotPVBonus;

                        // Increase the reduction for moves if we have a transposition table move since it's most likely
                        // the best move in the position and the others are likely worse
                        if (ttMove) r += LMRTTMoveBonus;

                        // If we are not improving positionally, we can afford to reduce the search depth further
                        if (!improving) r += LMRNotImprovingBonus;

                        // If our move gave check, we should try to reduce the search depth less as the move may be
                        // tactical and in certain cases, extend the search depth instead
                        if (Board.Checked<OColor>()) r -= LMRGaveCheckPenalty;

                        if (quiet) {
                            // Increase reduction for bad history moves and reduce for good history moves (possibly
                            // extending the search depth)
                            const int16_t history = History[Color][movingPiece][move.To()];
                            r -= history * LMRHistoryPartition * LMRHistoryWeight / HistoryLimit;
                        }

                        // Divide by the granularity factor to ensure that the fixed-point reduction is correctly
                        // mapped to discrete reduction
                        r /= LMRQuantization;

                        evaluation = -PVS<OColor, false, false>(
                            ply + 1,
                            std::clamp<int16_t>(depth - r, 1, depth),
                            -alpha - 1,
                            -alpha
                        );
                    } else evaluation = alpha + 1;

                    if (evaluation > alpha) {
                        evaluation = -PVS<OColor, false, false>(ply + 1, depth - 1, -alpha - 1, -alpha);

                        if (evaluation > alpha && evaluation < beta)
                            evaluation = -PVS<OColor, true, false>(ply + 1, depth - 1, -beta, -alpha);
                    }
                }

                UndoMove<true>(state, move);

                if (evaluation < beta) {
                    if      (capture) searchedCaptures.Push(move);
                    else if ( quiet ) searchedQuiets  .Push(move);
                }

                uint64_t moveNodes = 0;

                if (Root && ThreadType == Main) {
                    moveNodes = GetNodes() - nodesBeforeMove;
                    RootNodes += moveNodes;
                }

                if (evaluation <= bestEvaluation) continue;

                bestEvaluation = evaluation;

                if (Root && ThreadType == Main) BestMoveNodes = moveNodes;

                if (evaluation <= alpha) continue;

                alpha = evaluation;

                ttEntryNew.Type = Exact;
                ttEntryNew.Move =  move;

                if (ThreadType == Main && PV && !Stopped()) {
                    // The main thread is responsible for updating the PV Table in PV branches. We should be careful
                    // not to do this if the search was stopped, otherwise we may corrupt the PV Table

                    PVTable[ply].PV[ply] = move;

                    for (uint8_t nthPly = ply + 1; nthPly < PVTable[ply + 1].Ply; nthPly++)
                        PVTable[ply].PV[nthPly] = PVTable[ply + 1].PV[nthPly];

                    PVTable[ply].Ply = PVTable[ply + 1].Ply;
                }

                if (evaluation < beta) continue;

                if (!Stopped()) {
                    if        (capture) {
                        // Capture History Updates (Asymmetric Approach):
                        //
                        // We should give a bonus for the capture that caused a beta cut-off
                        UpdateCaptureHistory<Color, true>(move, depth);
                    } else if ( quiet ) {
                        // Killer Updates:
                        //
                        // Update the Killer table if a quiet move caused a beta cut-off to ensure we search this move
                        // earlier in the future

                        // If Killer Move 0 is different from the current move:
                        //    Killer Move 0 -> Killer Move 1
                        //   Current Move   -> Killer Move 0
                        if (!Killer[0][ply].SameIdentity(move)) {
                            Killer[1][ply] = Killer[0][ply];
                            Killer[0][ply] = move;
                        }

                        // History Updates:
                        //
                        // We should update histories (raise the move that caused the beta cut-off and diminish the
                        // moves that didn't) so that we search them earlier in the future and can use their values to
                        // make better reduction & pruning decisions and order similar moves earlier

                        // Bonus for the quiet that caused a beta cut-off
                        UpdateHistory<Color, true>(move, depth, ply);

                        // Malus for all other quiets as they didn't cause a beta cut-off
                        for (const auto m : searchedQuiets) UpdateHistory<Color, false>(m, depth, ply);
                    }

                    // Capture History Updates (Asymmetric Approach):
                    //
                    // We should apply a malus for all captures that didn't cause a beta cut-off. We generally consider
                    // captures to be good so if they aren't good (especially if they're worst than a quiet) then we
                    // should apply a malus to them
                    for (const auto m : searchedCaptures) UpdateCaptureHistory<Color, false>(m, depth);
                }

                ttEntryNew.Type = Beta;
                break;
            }

            ttEntryNew.Evaluation = CompressScore(bestEvaluation, ply);

            if (!Stopped()) {
                const bool ttMoveIsTactical = ttEntryNew.Move.Capture() || ttEntryNew.Move.Promotion() != NAP;

                // Correction History Update:
                //
                // As long as we don't expect the neural network evaluation to be off (we expect that in tactical
                // positions), we should update our correction history so we may remember the misguidedness of the
                // neural network evaluation and can make better decisions in the future
                if (!checked && !IsMate(bestEvaluation) && (ttEntryNew.Type == Alpha || !ttMoveIsTactical)) {
                    if (ttHit && ttEntry.Type == Exact) scaledEvaluation = ScaleEvaluation<Color>();

                    const Score correctedEvaluation = CorrectEvaluation<Color>(scaledEvaluation, ply);

                    if ((ttEntryNew.Type != Beta  || bestEvaluation > correctedEvaluation) &&
                        (ttEntryNew.Type != Alpha || bestEvaluation < correctedEvaluation))
                        UpdateCorrectionHistory<Color>(bestEvaluation - correctedEvaluation, depth, ply);
                }

                // Transposition Table Writing:
                //
                // As long as the search has not stopped, we should try to insert/replace the transposition table entry
                // with the new entry as it is most likely more relevant than the old entry
                TryReplaceTT(hash, ttEntryNew);
            }

            return bestEvaluation;
        }

        template<Color Color, bool PV>
        Score Quiescence(const uint8_t ply, Score alpha, const Score beta)
        {
            // Opponent's color for recursive calls
            constexpr auto OColor = Opposite(Color);

            // The main thread is responsible for ensuring that the correct selective depth is reported
            if (ThreadType == Main && PV) SelectiveDepth = std::max(SelectiveDepth, ply);

            // We should stop searching if we have reached the maximum depth as otherwise we may start accessing
            // memory we haven't allocated
            if (ply >= MaxDepth) [[unlikely]]
                return Board.Checked<Color>() ? Draw : CorrectEvaluation<Color>(ScaleEvaluation<Color>(), ply);

            if (!PV) {
                // Transposition Table Reading:
                //
                // We can check if the current position has been searched before, and if it has, then there most likely
                // exists a transposition entry - if the entry is valid, depending on the bounds of the entry, we can
                // return the evaluation from the entry. We do not check for the entry's depth here, as we are already
                // at a non-positive depth and all entries are going to be at least deeper than this depth. We also do
                // not do this in PV branches as even a slight inaccuracy due to hash collisions or other factors can
                // cause us to miss a good move

                const ZobristHash hash = Board.Zobrist();

                const SearchTranspositionEntry ttEntry = TT[hash];

                if (ttEntry.Hash == CompressHash(hash)) {
                    const Score ttEvaluation = DecompressScore(ttEntry.Evaluation, ply);

                    if (ttEntry.Type == Exact                         ) return ttEvaluation;
                    if (ttEntry.Type == Beta  && ttEvaluation >= beta ) return ttEvaluation;
                    if (ttEntry.Type == Alpha && ttEvaluation <= alpha) return ttEvaluation;
                }
            }

            // Static Evaluation:
            //
            // In Quiescence search, we use the neural network evaluation directly as the static evaluation
            const Score staticEvaluation = CorrectEvaluation<Color>(ScaleEvaluation<Color>(), ply);

            // Window Adjustment:
            //
            // If our static evaluation is already better than our upper bound (beta), we directly have a beta cut-off
            // and can return immediately without searching any moves. If that is not the case, we should adjust our
            // lower bound (alpha) to be the maximum of our current lower bound and the static evaluation, as we only
            // want to look for tactical sequences that improve our position
            if (staticEvaluation >= beta) return beta;
            if (staticEvaluation > alpha) alpha = staticEvaluation;

            using MoveList = OrderedMoveList<Color, true>;

            MoveList moves (Board, ply, Killer, NullHistory, NullHistory, CaptureHistory);

            Score bestEvaluation = staticEvaluation;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move = moves[i];

                // Static Exchange Evaluation (SEE) Pruning:
                //
                // SEE is essentially an evaluation that determines if an exchange of pieces is materially favorable for
                // us or not, and if it is not, then that tactical sequence is not worth searching further, and we can
                // prune that branch entirely
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
        PreviousState DoMove(const Move move, const uint8_t ply)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            const bool resetHalfMoveCounter = move.Capture() || Board[move.From()].Piece() == Pawn;

            Stack[ply + 1].HalfMoveCounter = resetHalfMoveCounter ? 0 : Stack[ply].HalfMoveCounter + 1;

            Stack[ply].       Move = move;
            Stack[ply].PieceToMove = Board[move.From()].Piece();

            const PreviousState state = Board.Move<MT>(move, ThreadId);
            IncrementNodes();

            const ZobristHash hash = Board.Zobrist();

            TT.Prefetch(hash);

            if (UpdateRepetitionHistory) Repetition.Push(hash);

            return state;
        }

        template<bool UpdateRepetitionHistory>
        void UndoMove(const PreviousState state, const Move move)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            Board.UndoMove<MT>(state, move, ThreadId);

            if (UpdateRepetitionHistory) Repetition.Pop();
        }

        template<Color Color, bool Increase>
        void UpdateHistory(const Move move, const int16_t depth, const uint8_t ply)
        {
            const int16_t bonus = std::clamp<int32_t>(HistoryMultiplier * depth - HistoryShiftDown, 0, HistoryLimit);

            int16_t& history = History[Color][Board[move.From()].Piece()][move.To()];

            history += bonus * (Increase ? 1 : -1) - history * bonus / HistoryLimit;

            if (!Stack[ply - 1].Move) return;

            int16_t& continuation = ContinuationHistory[Stack[ply - 1].PieceToMove][Stack[ply - 1].Move.To()]
                                    [Color][Board[move.From()].Piece()][move.To()];

            continuation += bonus * (Increase ? 1 : -1) - continuation * bonus / HistoryLimit;
        }

        template<Color Color, bool Increase>
        void UpdateCaptureHistory(const Move move, const int16_t depth)
        {
            const int16_t bonus = std::clamp<int32_t>(
                CaptureHistoryMultiplier * depth - CaptureHistoryShiftDown, 0, HistoryLimit
            );

            const auto targetPiece = move.EnPassant() ? Pawn : Board[move.To()].Piece();

            int16_t& history = CaptureHistory[Color][Board[move.From()].Piece()][move.To()][targetPiece];

            history += bonus * (Increase ? 1 : -1) - history * bonus / HistoryLimit;
        }

        template<Color Color>
        Score CorrectEvaluation(const Score evaluation, const uint8_t ply) const
        {
            const       ZobristHash     minor = Board.ZobristMinor();
            const Array<ZobristHash, 2> major = {
                Board.ZobristMajor(White), Board.ZobristMajor(Black)
            };

            const int16_t minorHistory = MinorCorrectionHistory[Color][minor % CorrectionHistorySize];

            const Array<int16_t, 2> majorHistory = {
                MajorCorrectionHistory[White][Color][major[White] % CorrectionHistorySize],
                MajorCorrectionHistory[Black][Color][major[Black] % CorrectionHistorySize]
            };

            const int32_t minorCorrection =                minorHistory                 * CorrectionHistoryMinorWeight;
            const int32_t majorCorrection = (majorHistory[White] + majorHistory[Black]) * CorrectionHistoryMajorWeight;

            int32_t continuationCorrection = 0;

            if (ply >= 2 && Stack[ply - 2].Move && Stack[ply - 1].Move) {
                const auto   ourPreviousPiece = Stack[ply - 2].PieceToMove;
                const auto theirPreviousPiece = Stack[ply - 1].PieceToMove;

                const auto   ourPreviousTarget = Stack[ply - 2].Move.To();
                const auto theirPreviousTarget = Stack[ply - 1].Move.To();

                const int16_t correctionContinuationHistory = CorrectionContinuationHistory[Color]
                    [  ourPreviousPiece][  ourPreviousTarget]
                    [theirPreviousPiece][theirPreviousTarget];

                continuationCorrection = correctionContinuationHistory * CorrectionHistoryContinuationWeight;
            }

            const int32_t correction = minorCorrection + majorCorrection + continuationCorrection;

            return std::clamp<Score>(
                evaluation + correction / CorrectionHistoryQuantization,
                -MateInMaxDepth + 1,
                 MateInMaxDepth - 1
            );
        }

        template<Color Color>
        void UpdateCorrectionHistory(const Score difference, const int16_t depth, const uint8_t ply)
        {
            const int32_t bonus = std::clamp<int32_t>(
                difference * depth / CorrectionHistoryDepthDivisor,
                -CorrectionHistoryMaximumBonus,
                 CorrectionHistoryMaximumBonus
            );

            const ZobristHash minor = Board.ZobristMinor();
            int16_t& minorCorrection = MinorCorrectionHistory[Color][minor % CorrectionHistorySize];

            minorCorrection += bonus - minorCorrection * abs(bonus) / CorrectionHistoryLimit;

            for (const auto color : { White, Black }) {
                const ZobristHash major = Board.ZobristMajor(color);
                int16_t& majorCorrection = MajorCorrectionHistory[color][Color][major % CorrectionHistorySize];

                majorCorrection += bonus - majorCorrection * abs(bonus) / CorrectionHistoryLimit;
            }

            if (ply >= 2 && Stack[ply - 2].Move && Stack[ply - 1].Move) {
                const auto   ourPreviousPiece = Stack[ply - 2].PieceToMove;
                const auto theirPreviousPiece = Stack[ply - 1].PieceToMove;

                const auto   ourPreviousTarget = Stack[ply - 2].Move.To();
                const auto theirPreviousTarget = Stack[ply - 1].Move.To();

                int16_t& correctionContinuation = CorrectionContinuationHistory[Color]
                    [  ourPreviousPiece][  ourPreviousTarget]
                    [theirPreviousPiece][theirPreviousTarget];

                correctionContinuation += bonus - correctionContinuation * abs(bonus) / CorrectionHistoryLimit;
            }
        }

        template<Color Color>
        [[clang::noinline]]
        Score ScaleEvaluation() const
        {
            const BitBoard pawn   = Board.PieceBoard(Pawn  , White) | Board.PieceBoard(Pawn  , Black);
            const BitBoard knight = Board.PieceBoard(Knight, White) | Board.PieceBoard(Knight, Black);
            const BitBoard bishop = Board.PieceBoard(Bishop, White) | Board.PieceBoard(Bishop, Black);
            const BitBoard rook   = Board.PieceBoard(Rook  , White) | Board.PieceBoard(Rook  , Black);
            const BitBoard queen  = Board.PieceBoard(Queen , White) | Board.PieceBoard(Queen , Black);

            const int32_t weightedMaterial = MaterialScalingQuantization                                             +
                                             (static_cast<int32_t>(Count(pawn  )) -16) * MaterialScalingWeightPawn   +
                                             (static_cast<int32_t>(Count(knight)) - 4) * MaterialScalingWeightKnight +
                                             (static_cast<int32_t>(Count(bishop)) - 4) * MaterialScalingWeightBishop +
                                             (static_cast<int32_t>(Count(rook  )) - 4) * MaterialScalingWeightRook   +
                                             (static_cast<int32_t>(Count(queen )) - 2) * MaterialScalingWeightQueen  ;

            return (Evaluation::Evaluate(Color, ThreadId) * weightedMaterial) / MaterialScalingQuantization;
        }

        static void TryReplaceTT(const ZobristHash hash, const SearchTranspositionEntry nEntry)
        {
            const SearchTranspositionEntry pEntry = TT[hash];

            if (nEntry.Type == Exact || nEntry.Hash != pEntry.Hash ||
               (pEntry.Type == Alpha &&
                nEntry.Type == Beta) ||
                nEntry.Depth > pEntry.Depth - TTReplacementDepthMargin)
                TT[hash] = nEntry;
        }

    };

    class ParallelTaskPool
    {

        using ParallelTask = SearchTask<Parallel>;

        std::vector<ParallelTask> SearchTaskPool;
        std::vector<ThreadedTask> ThreadTaskPool;

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

            SearchTaskPool.reserve(TaskCount);
            ThreadTaskPool.reserve(TaskCount);
        }

        void Clear()
        {
            SearchTaskPool.clear();
            ThreadTaskPool.clear();
        }

        size_t Size() const { return TaskCount; }

        void Fill(Limit& l, Board& b, RepetitionStack& r, const uint8_t hmc)
        {
            for (size_t i = 0; i < TaskCount; i++) SearchTaskPool.emplace_back(l, b, r, hmc, i + 1);
        }

        void Execute()
        {
            for (auto& task : SearchTaskPool) ThreadTaskPool.emplace_back(ThreadPool.Execute(
                [&task] -> void { task.IterativeDeepening(); }
            ));
        }

        void Stop()
        {
            for (auto &task : SearchTaskPool) task.Stop();
            for (auto &task : ThreadTaskPool) task.Wait();
        }

        auto Tasks() const { return std::views::zip(SearchTaskPool, ThreadTaskPool); }

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

                for (const auto& [task, _] : ParallelTaskPool.Tasks()) event.Nodes += task.GetNodes();

                return MainEventHandler::HandleIterativeDeepeningIterationCompletion(event);
            }

            static void HandleIterativeDeepeningCompletion(const IterativeDeepeningCompletionEvent& e)
            { MainEventHandler::HandleIterativeDeepeningCompletion(e); }

        };

        using MainSearchTask = SearchTask<Main, ThreadedSearchTaskPassthroughHandler>;

        static inline MainSearchTask MainTask;

        static inline Atomic<bool> Searching { false };

        static void Run(Limit& l, Board& b, RepetitionStack& r, const uint8_t hmc)
        {
            if (Searching.Exchange(true, MemoryOrder::acq_rel)) return;

            // Symmetric MultiProcessing (SMP):
            //
            // Relevant links:
            // - http://en.wikipedia.org/wiki/Symmetric_multiprocessing
            // - https://www.chessprogramming.org/Lazy_SMP
            //
            // Assuming there is more than one thread available for the search, a SMP approach is used to get a much
            // more accurate search. This approach relies on constructive and destructive interference of multiple
            // threads through the transposition table shared between them - the main thread is running the main search
            // task, while the other threads are running parallel search tasks that are constructively and destructively
            // interfering with the main search task (and with each other).
            //
            // Destructive interference happens when one task overwrites the transposition table entry another task is
            // about to read - this prevents the task from reading the entry and forces it to search the position again,
            // potentially disabling certain pruning techniques and enabling others. Regardless, the search space
            // explored by the task gets widened, and the search is more accurate, however, searching the extra branches
            // causes the search to take longer to complete; a consequence of destructive interference.
            //
            // Constructive interference happens when one task provides a transposition table entry that another task
            // is about to read - this allows the task to read the entry and use it to speed through certain branches by
            // returning directly from the transposition table if possible. Where this isn't possible, the task can use
            // the entry's evaluation as the static evaluation for that branch, enabling certain pruning techniques and
            // disabling others, once again widening the search space explored by the task. Furthermore, the provided
            // entry's move can be used to speed through the branch since the move policy ensures that the move is
            // the first move in the move list. This can allow the search for that branch to complete much faster,
            // overall reducing the search time, mitigating the destructive interference's negative impact on the
            // search time.
            //
            // Together, the search space is considerably widened, making the search more accurate, while the search
            // time remains relatively the same on average. This leads the engine to find better moves in the same
            // amount of time and avoid some pitfalls of the heuristical pruning, reduction, and search techniques used

            if (ParallelTaskPool.Size()) {
                ParallelTaskPool.Fill(l, b, r, hmc);
                ParallelTaskPool.Execute();
            }

            ReplaceInline(MainTask, l, b, r, hmc, 0);

            ThreadPool.Execute(
                [] -> void
                {
                    MainTask.IterativeDeepening();

                    // The main thread is responsible for ensuring that it stops all the parallel tasks when it has
                    // concluded searching
                    if (ParallelTaskPool.Size()) {
                        ParallelTaskPool.Stop();
                        ParallelTaskPool.Clear();
                    }

                    Searching.Store(false, MemoryOrder::release);
                    Searching.NotifyAll();
                }
            );
        }

    };

} // StockDory

#endif //STOCKDORY_SEARCH_H
