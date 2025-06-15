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

#include "Common.h"
#include "OrderedMoveList.h"
#include "TranspositionTable.h"
#include "TunableParameter.h"

namespace StockDory
{

    enum SearchTranspositionEntryType : uint8_t
    {

        UnknownType,

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

        // Identity
        CompressedHash Hash {};

        // Evaluation
        CompressedScore Evaluation       = None;
        CompressedScore StaticEvaluation = None;

        // Move
        Move Move {};

        // Metadata
        uint8_t   Depth {};
        EntryType Type  {};

    };

    inline TranspositionTable<SearchTranspositionEntry> TT (16 * MB);

    inline auto LMRTable =
    [] -> Array<int32_t, MaxDepth, MaxMove>
    {
        const auto formula = [](const uint8_t depth, const uint8_t move) -> int32_t
        {
            const int32_t value = (std::log(depth) * std::log(move) / 2 - 0.2) * LMRQuantization;

            return value / LMRQuantization > 0 ? value : 0;
        };

        Array<int32_t, MaxDepth, MaxMove> temp {};

        for (uint8_t depth = 1; depth < MaxDepth; depth++)
        for (uint8_t move  = 1; move  < MaxMove ;  move++) temp[depth][move] = formula(depth, move);

        return temp;
    }();

    class SearchStack
    {

        public:
        struct Frame
        {

            // Evaluation
            Score RawStaticEvaluation = None;
            Score    StaticEvaluation = None;

            // Misc
            uint8_t HalfMoveCounter = 0;

        };

        private:
        constexpr static size_t Padding = 8;

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
                // If we are in the main thread, we should see if we have a single move in this position. If that's the
                // case, we can save time by searching far less (ideally just a few depths) - saving time for when we
                // have more choices
                SearchSingleMoveTimeOptimization();

                // If we are in the main thread, we need to start the limit state such that if it's timed, we can track
                // the time elapsed and make sure we don't exceed the time allocation
                Limit.Start();
            }

            // Load the board state for evaluation purposes (this needs to be done for all threads with their respective
            // thread IDs as each thread has its own evaluation state)
            Board.LoadForEvaluation(ThreadId);

            IDepth = 1;
            while (!Limit.Crossed(Nodes, IDepth)) {
                const Move lastBestMove = BestMove;

                if (Board.ColorToMove() ==   White)
                     Evaluation = Aspiration<White>(IDepth);
                else Evaluation = Aspiration<Black>(IDepth);

                // In the case that the search was stopped, we should not use its result as the search is most likely
                // incomplete and the evaluation is not valid
                if (Status == SearchThreadStatus::Stopped) break;

                if (ThreadType == Main) {
                    // On the main thread, we need to fire events to notify handlers about the completion of the
                    // iterative deepening iteration and provide them with the results. Furthermore, if we are on the
                    // main thread, we should also try to see if our search is stable enough. If it is, we can avoid
                    // spending much more time on the search and instead save time for when we have multiple good moves
                    // to choose from

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
                // The main thread is responsible for notifying the handlers about the completion of the search,
                // providing them with the best move found

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
                    if (Limit.Crossed()) [[unlikely]] { Status = SearchThreadStatus::Stopped; }
                }

                // If the search was stopped, we should return a draw score immediately
                if (Status == SearchThreadStatus::Stopped) [[unlikely]] return Draw;

                // Window Fallback:
                //
                // If previous search and researches have failed to find a move within the window, then our window is
                // likely not capturing the relevant part of the search space. In this case, we should reset to a full
                // window and try again for future search iterations with a better understanding of the search space
                if (alpha < -AspirationWindowFallbackBound) alpha = -Infinity;
                if (beta  >  AspirationWindowFallbackBound) beta  =  Infinity;

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
                } else return bestEvaluation;
            }
        }

        template<Color Color, bool PV, bool Root>
        Score PVS(const uint8_t ply, int16_t depth, Score alpha, Score beta)
        {
            // Opponent's color for recursive calls
            constexpr auto OColor = Opposite(Color);

            if (ThreadType == Main) {
                // If we are in the main thread, we should regularly (every 4096 nodes) check if the search's limits
                // have been crossed. If they have, we should stop searching
                if ((Nodes & 4095) == 0 && Limit.Crossed()) [[unlikely]] { Status = SearchThreadStatus::Stopped; }
            }

            // If the search was stopped, we should return a draw score immediately
            if (Status == SearchThreadStatus::Stopped) [[unlikely]] return Draw;

            if (ThreadType == Main) {
                // The main thread is responsible for ensuring the PV Table is correctly updated with the right moves
                // and that the correct selective depth is reported

                PVTable[ply].Ply = ply;

                if (PV) SelectiveDepth = std::max(SelectiveDepth, ply);
            }

            // If we've exhausted our search depth, we should check if there are any tactical sequences available
            // over the horizon. In the case there are, our evaluation at this point is not truly accurate, and we must
            // get a more accurate evaluation by stepping through to the end of the tactical sequence - this is handled
            // by the Quiescence search
            if (depth <= 0) return Quiescence<Color, PV>(ply, alpha, beta);

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
                if (Stack[ply].HalfMoveCounter >= 100) return Draw;

                // Repetition:
                //
                // The repetition rule states that if the same position occurs three times, then the game is drawn. We
                // check this by storing the Zobrist Hashes of all positions we've seen in the current branch of the
                // search. We check if the current position's hash has been seen before N times, where N is equal to
                // the repetition limit (3 by default)
                if (Repetition.Found(hash, Stack[ply].HalfMoveCounter)) return Draw;

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

            // Transposition Table Reading:
            //
            // We can check if the current position has been searched before, and if it has, then there most likely
            // exists a transposition entry - if the entry is valid, depending on the quality of the entry, we can
            // return the evaluation from the entry. Even if the entry isn't of sufficient quality to return directly,
            // we can still search the move in the entry first, since it most likely is the best move in the position
            SearchTranspositionEntry& ttEntry      = TT[hash];
            Move                      ttMove       = {};
            bool                      ttHit        = false;
            Score                     ttEvaluation = None;

            if (ttEntry.Hash == CompressHash(hash)) {
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

            Score rawStaticEvaluation;
            Score    staticEvaluation;
            bool  improving          ;

            SearchTranspositionEntry staticTTEntry {
                .Hash = CompressHash(hash)
            };

            const bool checked = Board.Checked<Color>();

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
                rawStaticEvaluation = Stack[ply].RawStaticEvaluation = Stack[ply - 2].RawStaticEvaluation;
                   staticEvaluation = Stack[ply].   StaticEvaluation = Stack[ply - 2].   StaticEvaluation;

                // Positionally Improving:
                //
                // Typically, getting in check is a sign that we aren't positionally improving
                improving = false;

                // Check Extension:
                //
                // If we are under check, we should search this branch deeper since we need to find a good way to evade
                // the check
                depth += CheckExtension;

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
            // If we are not under check, we should use the static evaluation of the current position - this is done
            // via two possible ways:
            // - If we have a valid transposition table entry:
            //   - If the entry's bound is exact, it is representative of a search that is at least more accurate
            //     than whatever our neural network evaluation would provide
            //   - If the entry's bound is beta, it means that the evaluation caused a beta cut-off, but our
            //     neural network evaluation may be even more likely to cause a beta cut-off, so we should use
            //     whichever is more likely - in simple terms, use whichever evaluation is more optimistic
            //   - If the entry's bound is alpha, it means that the evaluation never exceeded alpha, but our neural
            //     network evaluation may be more unlikely to exceed alpha, so we should use whichever is more
            //     unlikely to exceed alpha - in simple terms, use whichever evaluation is more pessimistic
            // - If we do not have a valid transposition table entry, use the neural network evaluation
            if (ttHit) {
                rawStaticEvaluation = ttEntry.StaticEvaluation;

                if (rawStaticEvaluation == None) {
                    rawStaticEvaluation = Evaluation::Evaluate(Color, ThreadId);

                    staticTTEntry.StaticEvaluation = rawStaticEvaluation;
                }

                staticEvaluation = ScaleEvaluation(rawStaticEvaluation);

                if (ttEntry.Type == Exact) staticEvaluation =                                   ttEvaluation ;
                if (ttEntry.Type == Beta ) staticEvaluation = std::max<Score>(staticEvaluation, ttEvaluation);
                if (ttEntry.Type == Alpha) staticEvaluation = std::min<Score>(staticEvaluation, ttEvaluation);
            } else {
                rawStaticEvaluation = Evaluation::Evaluate(Color, ThreadId);

                staticTTEntry.StaticEvaluation = rawStaticEvaluation;

                staticEvaluation = ScaleEvaluation(rawStaticEvaluation);
            }

            TryReplaceTT(ttEntry, staticTTEntry);

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
                // for sure. However, in most cases, static evaluation is a good enough approximation and representative
                // of an actual search
                if (depth < ReverseFutilityMaximumDepth && abs(beta) < Mate) {
                    const Score     depthMargin = depth     * ReverseFutilityDepthFactor;
                    const Score improvingMargin = improving * ReverseFutilityImprovingFactor;

                    if (staticEvaluation - depthMargin + improvingMargin >= beta) return beta;
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
                if (!Root && depth >= NullMoveMinimumDepth && staticEvaluation >= beta) {
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

                    const PreviousStateNull state = Board.Move();

                    const auto evaluation = -PVS<OColor, false, false>(
                        ply + 1,
                        reducedDepth,
                        -beta,
                        -beta + 1
                    );

                    Board.UndoMove(state);

                    if (evaluation >= beta) return beta;
                }
            }

            // We continue from here if we are in check
            Checked:

            // Internal Iterative Reduction (IIR):
            //
            // If we are at a high enough depth but have no transposition table entry, we can reduce the depth of the
            // search by a small amount - good positions are normally reached by a lot of different sequences and
            // usually have a transposition table entry
            if (!PV && depth >= IIRMinimumDepth && !ttMove) depth -= IIRDepthReduction;

            using MoveList = OrderedMoveList<Color>;

            MoveList moves (Board, ply, Killer, History, ttMove);

            // Out of Moves:
            //
            // If we have no moves to search at this point, it is either because we are in checkmate or stalemate
            if (moves.Count() == 0) return checked ? LossIn(ply) : Draw;

            SearchTranspositionEntry ttEntryNew
            {
                .Hash             = CompressHash(hash),
                .Move             = ttMove,
                .Depth            = static_cast<uint8_t>(depth),
                .Type             = Alpha
            };

            ttEntryNew.StaticEvaluation = rawStaticEvaluation;

            const uint8_t lmpLastQuiet = LMPLastQuietBase +   depth * depth;
            const bool    doLMP        = !Root && !checked && depth <= LMPMaximumDepth;
            const bool    doLMR        =          !checked && depth >= LMRMinimumDepth;

            Score bestEvaluation = -Infinity;

            uint8_t quietMoves = 0;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move  = moves[i];

                const Piece movingPiece = Board[move.From()].Piece();
                const Piece targetPiece = Board[move.  To()].Piece();

                const bool quiet = targetPiece == NAP;

                quietMoves += quiet;

                // Futility Pruning (FP):
                //
                // FP is a pruning technique that prunes branches that are too bad for us to be worth searching further.
                // It is the opposite of RFP, and while trying to achieve the same goal as Razoring, it does so with a
                // very different approach - relying on the static evaluation and move policy. StockDory's Move Policy
                // ensures that tactical moves always come before quiet moves, so if we are at a point where we are
                // searching a quiet move, we can assume that all tactical moves have been searched already. Then, if
                // the static evaluation of the current position is significantly worse than our lower bound (alpha),
                // it is very unlikely that a non-tactical move will improve our position enough to exceed our lower
                // bound (alpha). Searching further in this branch is not going to change the outcome of this branch,
                // so we can stop early
                if (i > 0 && quiet) {
                    const Score margin = depth * FutilityDepthFactor;

                    if (staticEvaluation + margin <= alpha) break;
                }

                if (!PV) {
                    // Risky Pruning:
                    //
                    // The techniques below are risky pruning techniques that can cause us to miss some good moves.
                    // Doing this in PV branches can be disastrous, however, in non-PV branches, this is relatively safe
                    // to do. We can afford to miss some good moves in non-PV branches, as we are not that likely going
                    // to find the best move in these branches, mainly using the results of these branches to optimize
                    // search tree exploration

                    // Late Move Pruning (LMP):
                    //
                    // LMP is a pruning technique that allows us to prune branches that are too bad for us to be worth
                    // searching further. It is similar to FP and heavily relies on the move policy, working on the
                    // assumption that the move policy ensures that all the good moves are ordered before the bad ones
                    // and will be searched earlier. If we are at a point where we've even searched a few quiet moves,
                    // then it is very likely we've already searched the good moves and searching further is not going
                    // to change the outcome of this branch - so we can stop early
                    if (doLMP && quietMoves > lmpLastQuiet && bestEvaluation > -Infinity) break;
                }

                const PreviousState state = DoMove<true>(move, ply, quiet);

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
                    if (doLMR && i > LMRMinimumMoves) {
                        // Reduction values are determined by a formula that takes into account the current depth and
                        // move number. Current formula:
                        //
                        // r = floor((ln(depth) * ln(i) / 2 - 0.2) * LMRGranularityFactor)
                        int32_t r = LMRTable[depth][i];

                        // If we are not in a PV branch, we can afford to reduce the search depth further
                        if (!PV) r += LMRNotPVBonus;

                        // Increase the reduction for moves if we have a transposition table move since it's most likely
                        // the best move in the position and the others are likely worse
                        if (ttHit && (ttEntry.Type != Alpha && ttEntry.Type != None)) r += LMRTTMoveBonus;

                        // If we are not improving positionally, we can afford to reduce the search depth further
                        if (!improving) r += LMRNotImprovingBonus;

                        // If our last move gave check to the opponent, we should try to reduce the search depth less as
                        // the move may be tactical and in certain cases, extend the search depth instead
                        if (Board.Checked<OColor>()) r -= LMRGaveCheckPenalty;

                        // Increase reduction for bad history moves and reduce for good history moves (possibly
                        // extending the search depth)
                        const int16_t history = History[Color][movingPiece][move.To()];
                        r -= history / ((HistoryLimit / LMRHistoryPartition) / LMRHistoryWeight);

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

                if (evaluation <= bestEvaluation) continue;

                bestEvaluation = evaluation;

                if (evaluation <= alpha) continue;

                alpha = evaluation;

                ttEntryNew.Type = Exact;
                ttEntryNew.Move =  move;

                if (ThreadType == Main && PV && Status != SearchThreadStatus::Stopped) {
                    // The main thread is responsible for updating the PV Table in PV branches. We should be careful
                    // not to do this if the search was stopped, otherwise we may corrupt the PV Table

                    PVTable[ply].PV[ply] = move;

                    for (uint8_t nthPly = ply + 1; nthPly < PVTable[ply + 1].Ply; nthPly++)
                        PVTable[ply].PV[nthPly] = PVTable[ply + 1].PV[nthPly];

                    PVTable[ply].Ply = PVTable[ply + 1].Ply;
                }

                if (evaluation < beta) continue;

                if (Status != SearchThreadStatus::Stopped && quiet) {
                    // Killer and History Table Updates:
                    //
                    // Update the Killer and History Table if a quiet move caused a beta cut-off to ensure we search
                    // this move earlier in the future. Make sure not to do this if the search was stopped, as we'll
                    // otherwise corrupt the Killer and history table

                    // If Killer Move 0 is different from the current move:
                    //    Killer Move 0 -> Killer Move 1
                    //   Current Move   -> Killer Move 0
                    if (Killer[0][ply] != move) {
                        Killer[1][ply] = Killer[0][ply];
                        Killer[0][ply] = move;
                    }

                    // Increase the history value for the current move in the history table
                    UpdateHistory<Color, true>(move, depth);

                    // Reduce the history value for all other quiet moves that were searched, since they didn't
                    // cause a beta cut-off
                    for (uint8_t q = 1; q < quietMoves; q++)
                        UpdateHistory<Color, false>(moves.UnsortedAccess(i - q), depth);
                }

                ttEntryNew.Type = Beta;
                break;
            }

            ttEntryNew.Evaluation = CompressScore(bestEvaluation, ply);

            // Transposition Table Writing:
            //
            // As long as the search has not stopped, we should try to insert/replace the transposition table entry
            // with the new entry as it is most likely more relevant than the old entry
            if (Status != SearchThreadStatus::Stopped) TryReplaceTT(ttEntry, ttEntryNew);

            return bestEvaluation;
        }

        template<Color Color, bool PV>
        Score Quiescence(const uint8_t ply, Score alpha, const Score beta)
        {
            // Opponent's color for recursive calls
            constexpr auto OColor = Opposite(Color);

            // The main thread is responsible for ensuring that the correct selective depth is reported
            if (ThreadType == Main && PV) SelectiveDepth = std::max(SelectiveDepth, ply);

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

                const SearchTranspositionEntry& ttEntry = TT[hash];

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
            const Score staticEvaluation = EvaluateScaled<Color>().Scaled;

            // Window Adjustment:
            //
            // If our static evaluation is already better than our upper bound (beta), we directly have a beta cut-off
            // and can return immediately without searching any moves. If that is not the case, we should adjust our
            // lower bound (alpha) to be the maximum of our current lower bound and the static evaluation, as we only
            // want to look for tactical sequences that improve our position
            if (staticEvaluation >= beta) return beta;
            if (staticEvaluation > alpha) alpha = staticEvaluation;

            using MoveList = OrderedMoveList<Color, true>;

            MoveList moves (Board, ply, Killer, History);

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
            const int16_t bonus = std::min<int16_t>(HistoryMultiplier * depth - HistoryShiftDown, HistoryLimit);

            int16_t& history = History[Color][Board[move.From()].Piece()][move.To()];

            history += bonus * (Increase ? 1 : -1) - history * bonus / HistoryLimit;
        }

        Score ScaleEvaluation(const Score raw) const
        {
            const BitBoard pawn   = Board.PieceBoard(Pawn  , White) | Board.PieceBoard(Pawn  , Black);
            const BitBoard knight = Board.PieceBoard(Knight, White) | Board.PieceBoard(Knight, Black);
            const BitBoard bishop = Board.PieceBoard(Bishop, White) | Board.PieceBoard(Bishop, Black);
            const BitBoard rook   = Board.PieceBoard(Rook  , White) | Board.PieceBoard(Rook  , Black);
            const BitBoard queen  = Board.PieceBoard(Queen , White) | Board.PieceBoard(Queen , Black);

            uint16_t weightedMaterial = Count(pawn  ) * MaterialScalingWeightPawn   +
                                        Count(knight) * MaterialScalingWeightKnight +
                                        Count(bishop) * MaterialScalingWeightBishop +
                                        Count(rook  ) * MaterialScalingWeightRook   +
                                        Count(queen ) * MaterialScalingWeightQueen  ;

            weightedMaterial += MaterialScalingQuantization - MaterialScalingWeightedStartValue;

            return (raw * weightedMaterial) / MaterialScalingQuantization;
        }

        struct ScaledEvaluation { Score Raw; Score Scaled; };

        template<Color Color>
        ScaledEvaluation EvaluateScaled() const
        {
            ScaledEvaluation evaluation {
                .Raw = Evaluation::Evaluate(Color, ThreadId)
            };

            evaluation.Scaled = ScaleEvaluation(evaluation.Raw);

            return evaluation;
        }

        static void TryReplaceTT(SearchTranspositionEntry& pEntry, const SearchTranspositionEntry& nEntry)
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

                    // The main thread is responsible for ensuring that it stops all the parallel tasks when it has
                    // concluded searching
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
