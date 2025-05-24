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

#include "Move/OrderedMoveList2.h"

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

            if (depth >= AspirationWindowMinimumDepth) {
                // If we're past the required aspiration depth, it means previous searches
                // gave us a good ballpark for the evaluation. All future searches can start
                // with a much smaller window:
                //
                // [alpha, beta] = [evaluation - AspirationWindowSize, evaluation + AspirationWindowSize]
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

            SearchTranspositionEntry& ttEntry = TT[hash];
            Move                      ttMove  = {};
            bool                      ttHit   = false;

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
                // and should use our last recorded static evaluation from when we were
                // not in check. The static evaluation of a node in check may be wildly
                // misleading and incorrectly influence pruning
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
                    // transposition table, we can use it instead since it is likely more
                    // generally representative throughout the search
                    if      (ttEntry.Type == Beta ) staticEvaluation = std::max<Score>(staticEvaluation, nnEvaluation);
                    else if (ttEntry.Type == Alpha) staticEvaluation = std::min<Score>(staticEvaluation, nnEvaluation);
                }
            }

            Stack[ply].StaticEvaluation = staticEvaluation;

            // Improving is calculated on the difference between the current static evaluation
            // and our last recorded static evaluation from when we were not in check.
            // Our last recorded static evaluation would in normal conditions be the one from two
            // plies ago. However, if two plies ago we were in check, then the static evaluation
            // from two plies ago will be the one from four plies ago. This is recursive, so if
            // we were in check four plies ago, then the static evaluation from six plies ago will
            // be used, and so on. This happens implicitly because earlier if we're in check, we
            // set the static evaluation to the one from two plies ago.
            //
            // Stack[ply - 2].StaticEvaluation will always be our last recorded static evaluation
            // from when we were not in check
            improving = Stack[ply].StaticEvaluation > Stack[ply - 2].StaticEvaluation;

            if (!PV) {
                // We want to avoid potentially risky pruning in PV branches, since they are
                // the most important branches to search. In non-PV branches, we can prune
                // more aggressively since we can afford to miss some good moves - albeit,
                // the goal is to not miss any good moves

                // Reverse Futility Pruning:
                //
                // If the static evaluation is significantly better than what we expect,
                // it's almost certain that this branch is good enough and not worth searching
                // deeper - there's likely little the opponent can do to improve their position,
                // and we'll only waste time searching deeper
                if (depth < ReverseFutilityMaximumDepth && abs(beta) < Mate) {
                    // The lower the depth, the lesser the margin we can use. However, to prevent
                    // overestimation of the difference, we consider if we've been improving our
                    // static evaluation or not. If that's the case, we need to use a bigger margin
                    // since the static evaluation may be inflated
                    const Score margin = depth * ReverseFutilityDepthFactor +
                                     improving * ReverseFutilityImprovingFactor;

                    // This currently returns beta, but it might be better to return the static evaluation
                    // instead, since it would be more accurate. This is essentially the difference between
                    // fail-hard and fail-soft: fail-hard returns beta, while fail-soft returns the static
                    // evaluation
                    if (staticEvaluation - margin >= beta) return beta;
                }

                // Razoring:
                //
                // If we're near the end of the branch, and it's very unlikely that we will get better
                // than the lower bound, then doing a full search is usually not worth it since only
                // tactical moves can significantly improve the position. Thus, we jump into Quiescence
                // Search to look for good tactical sequences (should there exist)
                if (depth == RazoringDepth && staticEvaluation + RazoringEvaluationMargin < alpha)
                    return Quiescence<Color, false>(ply, alpha, beta);

                // Null Move Pruning:
                //
                // In chess, principally doing something is likely always better than doing nothing; if
                // even after doing nothing, we are better than our upper bound, then, if we were to do
                // something, we would definitely be better than our upper bound. Thus, it doesn't make
                // sense to evaluate such branches further, since we know they will always be good for us
                if (!Root && depth >= NullMoveMinimumDepth && staticEvaluation >= beta) {
                    // The reduced depth is calculated as follows:
                    //   depth
                    // - ( MinimumReduction
                    //   + ScalingDepthReduction
                    //   + ScalingEvaluationReduction
                    //   )
                    //
                    // where:
                    // MinimumReduction           = NullMoveMinimumReduction
                    // ScalingDepthReduction      = depth / NullMoveDepthFactor
                    // ScalingEvaluationReduction = (staticEvaluation - beta) / NullMoveEvaluationFactor

                    const int16_t reducedDepth = depth
                        - (  NullMoveMinimumReduction
                           + static_cast<int16_t>(depth / NullMoveDepthFactor)
                           + static_cast<int16_t>((staticEvaluation - beta) / NullMoveEvaluationFactor)
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

            // Internal Iterative Reduction:
            //
            // Given this node was on the PV branch but still did not have a transposition table entry,
            // it can be assumed that the position is likely not very promising, and thus we can reduce
            // the search depth a bit to save time
            if (depth >= IIRMinimumDepth && !ttHit) depth -= IIRDepthReduction;

            using MoveList = OrderedMoveList<Color>;

            MoveList moves (Board, ply, Killer, History, ttMove);

            // If we have no moves to search, it means we're either in a checkmate or stalemate node.
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

            // Keep track of the number of quiet moves we've seen so far - useful to enable
            // certain pruning and reductions
            uint8_t quietMoves = 0;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move  = moves[i];
                const bool quiet = Board[move.To()].Piece() == NAP;

                quietMoves += quiet;

                // Futility Pruning:
                //
                // If we're starting to search quiet moves, it's likely that we've already searched through
                // all the tactical moves. If the static evaluation is considerably worse than our lower bound,
                // then we can assume that the quiet moves will likely not improve our position enough to
                // matter, and it's better to stop searching this branch
                if (i > 0 && quiet) {
                    const Score margin = depth * FutilityDepthFactor;

                    if (staticEvaluation + margin <= alpha) break;
                }

                if (!PV) {
                    // These pruning techniques are too risky to use in PV nodes, since they can
                    // potentially prune the only good move that leads to a win, but in non-PV nodes
                    // we can afford to miss some good moves

                    // Late Move Pruning:
                    //
                    // If we've already seen enough quiet moves, then it's likely that future quiet
                    // moves will improve our position any more than the previous ones did - so let's
                    // avoid searching further
                    if (doLMP && quietMoves > lmpLastQuiet && bestEvaluation > -Infinity) break;
                }

                const PreviousState state = DoMove<true>(move, ply, quiet);

                Score evaluation = 0;

                // Principle Variation Search:
                //
                // This is an enhancement over the traditional Alpha-Beta Search algorithm and relies on the accuracy
                // of the move policy. It assumes that the best move is likely the first move in the move list, so we
                // search it without any reductions and with a full window [alpha, beta].
                //
                // All future moves are most likely not the best move, wso we can search them with varying reductions
                // and use a null window [alpha, alpha + 1] to see if they have any potential. In the case that they
                // do, it's likely that they are part of a Principle Variation branch, so we can do a full window
                // search on them as well
                if (i == 0) evaluation = -AlphaBeta<OColor, PV, false>(ply + 1, depth - 1, -beta, -alpha);
                else {
                    // All other moves can be searched with varying reductions

                    // Late Move Reduction:
                    //
                    // We can reduce the search depth for moves that appear later in the move list, since
                    // they are less likely to be the best move according to the move policy where the
                    // best moves appear earlier in the move list
                    if (doLMR && i >= LMRMinimumMoves) {
                        // Base reduction: floor(ln(depth) * ln(move) / 2 - 0.2)
                        int16_t r = LMRReductionTable[depth][i];

                        // If we're not in a PV node, reduce further since we can afford a shallower search
                        if (!PV) r++;

                        // If we aren't improving our position, it's unlikely that the branch we're in will
                        // yield any further improvements, so we can reduce further
                        if (!improving) r++;

                        // If the move we did gave the opponent a check, it's likely that this move may be
                        // a part of a tactical sequence, so we should reduce less (potentially even extending)
                        if (Board.Checked<OColor>()) r--;

                        // We do a null window search, so if LMR fails and does find a somewhat promising move,
                        // we can do a regular Principle Variation Search on it
                        evaluation = -AlphaBeta<OColor, false, false>(
                            ply + 1,
                            std::max<int16_t>(depth - r, 1),
                            -alpha - 1,
                            -alpha
                        );
                    } else {
                        // Set the evaluation to alpha + 1, which suggests that LMR failed, and we should do
                        // a regular Principle Variation Search on this move
                        evaluation = alpha + 1;
                    }

                    if (evaluation > alpha) {
                        // This means that the either LMR failed - found a promising move - or that it was
                        // not appropriate to do LMR in the first place. Regardless, we should perform a
                        // regular Principle Variation Search on this move

                        // First, try to do a null window search with no reductions
                        evaluation = -AlphaBeta<OColor, false, false>(ply + 1, depth - 1, -alpha - 1, -alpha);

                        if (evaluation > alpha && evaluation < beta) {
                            // If the null window search found a promising move, we should do a full
                            // Principle Variation Search on it

                            evaluation = -AlphaBeta<OColor, true, false>(ply + 1, depth - 1, -beta, -alpha);
                        }
                    }
                }

                UndoMove<true>(state, move);

                // If the evaluation is worse than an evaluation we previously had, then this is not the best move
                if (evaluation <= bestEvaluation) continue;

                bestEvaluation = evaluation;

                if (evaluation <= alpha) continue;

                // Update our lower bound with the best evaluation we found so far, since this will be useful for
                // determining whether we are to continue searching this branch further or not
                alpha = evaluation;

                ttEntryNew.Type = Exact;
                ttEntryNew.Move =  move;

                if (ThreadType == Main && PV && Status != SearchThreadStatus::Stopped) {
                    // We only update the PV table in the main thread since it is responsible for maintaining
                    // the correct Principal Variation. The parallel threads do not need to worry about this -
                    // later if we support MultiPV, we can update the PV table in parallel threads. Furthermore,
                    // the PV table is only updated in PV branches, and only if the search wasn't stopped, since
                    // otherwise we may corrupt the PV table with incomplete data

                    PVTable[ply].PV[ply] = move;

                    for (uint8_t nthPly = ply + 1; nthPly < PVTable[ply + 1].Ply; nthPly++)
                        PVTable[ply].PV[nthPly] = PVTable[ply + 1].PV[nthPly];

                    PVTable[ply].Ply = PVTable[ply + 1].Ply;
                }

                if (evaluation < beta) continue;

                // We've just found a move that is better than our upper bound, so we can cause a beta cut-off,
                // but before we need to update our killers and history tables
                if (Status != SearchThreadStatus::Stopped && quiet) {
                    // Update the killers
                    if (Killer[0][ply] != move) {
                        Killer[1][ply] = Killer[0][ply];
                        Killer[0][ply] = move;
                    }

                    // Give a history bonus to the beta cut-off move
                    UpdateHistory<Color, true>(move, depth);

                    // Take away a history bonus from all the other moves to simulate gravity
                    for (uint8_t q = 1; q < quietMoves; q++) {
                        // Loop backwards, since this is better for cache
                        UpdateHistory<Color, false>(moves.UnsortedAccess(i - q), depth);
                    }
                }

                ttEntryNew.Type = Beta;

                // Beta cut-off occurred so we can short-circuit this branch
                break;
            }

            // Set the score in the transposition table entry
            ttEntryNew.Evaluation = CompressScore(bestEvaluation);

            // Try replacing the transposition table entry with the new one - we don't do this if the search
            // was stopped, since we don't want to replace the entry with an incomplete one and corrupt the
            // transposition table
            if (Status != SearchThreadStatus::Stopped) TryReplaceTTEntry(ttEntry, ttEntryNew);

            return bestEvaluation;
        }

        template<Color Color, bool PV>
        Score Quiescence(const uint8_t ply, Score alpha, Score beta)
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

            const Score staticEvaluation = Evaluation::Evaluate(Color);

            if (staticEvaluation >= beta) return beta;
            if (staticEvaluation > alpha) alpha = staticEvaluation;

            using MoveList = OrderedMoveList<Color, true>;


        }

        template<bool UpdateRepetitionHistory>
        PreviousState DoMove(const Move move, const uint8_t ply, const bool quiet = false)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            if (!quiet || Board[move.From()].Piece() == Pawn) {
                // The move was a capture or pawn move, so we must reset the half-move counter
                Stack[ply + 1].HalfMoveCounter = 0;
            } else
                // Otherwise, we increment the half-move counter
                Stack[ply + 1].HalfMoveCounter = Stack[ply].HalfMoveCounter + 1;

            const PreviousState state = Board.Move<MT>(move.From(), move.To(), move.Promotion());

            // Atomically increment the number of nodes searched - we use memory_order_relaxed to avoid
            // unnecessary synchronization overhead, since we don't care about the order of increments
            // and only care about the final count
            Nodes.fetch_add(1, std::memory_order_relaxed);

            const ZobristHash hash = Board.Zobrist();

            // We can prefetch the transposition table entry for this position since we will likely need it
            // for the next search frame
            TT.Prefetch(hash);

            // If repetitions can happen, we should push the hash to the repetition stack so we can check for
            // it later to ensure we don't evaluate a position as a win or loss when it is actually a draw
            if (UpdateRepetitionHistory) Repetition.Push(hash);

            return state;
        }

        template<bool UpdateRepetitionHistory>
        void UndoMove(const PreviousState state, const Move move)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            Board.UndoMove<MT>(state, move.From(), move.To());

            // If we previously pushed the hash to the repetition stack, we should pop it now
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
            // Replacement Strategy - Replace if:
            // - The new entry is an exact entry
            // - The new entry has a different hash than the previous entry
            // - The new entry is a beta entry and the previous entry is an alpha entry
            // - The new entry came from a search that less than 2 plies shallower than the previous entry

            if (nEntry.Type == Exact || nEntry.Hash != pEntry.Hash ||
               (pEntry.Type == Alpha &&
                nEntry.Type == Beta) ||
                nEntry.Depth > pEntry.Depth - TTReplacementDepthMargin)
                pEntry = nEntry;
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
