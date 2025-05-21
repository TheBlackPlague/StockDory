//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEARCH_H
#define STOCKDORY_SEARCH_H

#include "../Backend/Board.h"
#include "../Backend/Type/Move.h"
#include "../Backend/Type/Zobrist.h"

#include "EngineParameter.h"
#include "Evaluation.h"
#include "LogarithmicReductionTable.h"
#include "RepetitionHistory.h"
#include "SEE.h"
#include "Move/HistoryTable.h"
#include "Move/KillerTable.h"
#include "Move/OrderedMoveList.h"
#include "Move/PrincipleVariationTable.h"
#include "Time/TimeManager.h"

namespace StockDory
{

    struct Limit
    {

        private:
        uint64_t Nodes = 0xFFFFFFFFFFFFFFFF;
        uint8_t  Depth = MaxDepth / 2;

        public:
        constexpr Limit() = default;

        constexpr explicit Limit(const uint64_t nodes)
        {
            Nodes = nodes;
        }

        constexpr explicit Limit(const uint8_t depth)
        {
            Depth = depth;
        }

        bool BeyondLimit(const uint64_t nodes, const uint8_t depth) const
        {
            return nodes > Nodes || depth > Depth;
        }

    };

    class DefaultHandler
    {

        using PVEntry = PrincipleVariationEntry;

        public:
        static void HandleDepthIteration([[maybe_unused]] const uint8_t           depth,
                                         [[maybe_unused]] const uint8_t  selectiveDepth,
                                         [[maybe_unused]] const int32_t      evaluation,
                                         [[maybe_unused]] const uint64_t          nodes,
                                         [[maybe_unused]] const MS                 time,
                                         [[maybe_unused]] const PVEntry&             pv) {}

        static void HandleTTIteration([[maybe_unused]] const uint8_t           depth,
                                      [[maybe_unused]] const uint64_t          exact,
                                      [[maybe_unused]] const uint64_t     betaCutoff,
                                      [[maybe_unused]] const uint64_t alphaUnchanged) {}

        static void HandleBestMove([[maybe_unused]] const Move move) {}

    };

    template<class EventHandler = DefaultHandler>
    class Search
    {

        class SearchStopException final : public std::exception {};

        struct SearchStackEntry
        {

            int32_t StaticEvaluation = 0;
            uint8_t HalfMoveCounter  = 0;

        };

        Board       Board;
        TimeControl TC;

        RepetitionHistory Repetition = RepetitionHistory(0);

        PrincipleVariationTable PVTable = PrincipleVariationTable();
        KillerTable             KTable  = {};
        HistoryTable            HTable  = {};

        std::array<SearchStackEntry, MaxDepth> Stack = {};

        std::array<uint64_t, 3> TTHits = {};

        uint8_t SelectiveDepth = 0;

        uint64_t Nodes = 0;

        int32_t Evaluation = -Infinity;
        Move    BestMove   = NoMove;

        uint8_t BestMoveStability = 0;

        bool Stop = false;

        public:
        Search() = default;

        // ReSharper disable CppPassValueParameterByConstReference
        Search(const StockDory::Board       board, const TimeControl tc,
               const RepetitionHistory repetition, const uint8_t     hm)
            : Board(board), TC(tc), Repetition(repetition)
        {
            Stack[0].HalfMoveCounter = hm;
        }
        // ReSharper restore CppPassValueParameterByConstReference

        void IterativeDeepening(const Limit limit)
        {
            Board.LoadForEvaluation();

            TC.Start();

            int16_t currentDepth = 1;
            while (!limit.BeyondLimit(Nodes, currentDepth) && !TC.Finished<false>()) {
                const Move lastBestMove = BestMove;

                if (Board.ColorToMove() ==   White)
                     Evaluation = Aspiration<White>(currentDepth);
                else Evaluation = Aspiration<Black>(currentDepth);

                if (Stop) break;

                BestMove = PVTable[0].PV[0];

                BestMoveStabilityOptimisation(lastBestMove);

                EventHandler::HandleDepthIteration(
                    currentDepth,
                    SelectiveDepth,
                    Evaluation,
                    Nodes,
                    TC.Elapsed(),
                    PVTable[0]
                );
                EventHandler::HandleTTIteration(
                    currentDepth,
                    TTHits[Exact],
                    TTHits[BetaCutoff],
                    TTHits[AlphaUnchanged]
                );
                currentDepth++;
            }

            EventHandler::HandleBestMove(BestMove);
        }

        [[nodiscard]]
        uint64_t NodesSearched() const
        {
            return Nodes;
        }

        void ForceStop()
        {
            Stop = true;
        }

        private:
        void BestMoveStabilityOptimisation(const Move lastBestMove)
        {
            if (lastBestMove == BestMove) BestMoveStability = std::min(BestMoveStability + 1, 4);
            else BestMoveStability                          = 0;

            TimeManager::Optimize(TC, BestMoveStabilityOptimisationFactor[BestMoveStability]);
        }

        template<Color Color>
        int32_t Aspiration(const int16_t depth)
        {
            //region Window Setting
            int32_t alpha = -Infinity;
            int32_t beta  =  Infinity;

            if (depth > AspirationDepth) {
                alpha = Evaluation - AspirationSize;
                beta  = Evaluation + AspirationSize;
            }
            //endregion

            uint8_t research = 0;
            while (true) {
                //region Out of Time & Force Stop
                if (Stop || TC.Finished<true>()) [[unlikely]] { Stop = true; return Draw; }
                //endregion

                //region Reset Window
                if (alpha < -AspirationBound) alpha = -Infinity;
                if (beta  >  AspirationBound) beta  =  Infinity;
                //endregion

                // ReSharper disable once CppTooWideScopeInitStatement
                const int32_t bestEvaluation = AlphaBeta<Color, true, true>(0, depth, alpha, beta);

                //region Modify Window
                if (bestEvaluation <= alpha) {
                    research++;

                    alpha = std::max(alpha - research * research * AspirationDelta, -Infinity);
                } else if (bestEvaluation >= beta) {
                    research++;

                    beta  = std::min(beta  + research * research * AspirationDelta,  Infinity);

                    BestMove = PVTable[0].PV[0];
                } else return bestEvaluation;
                //endregion
            }
        }

        template<Color Color, bool Pv, bool Root>
        int32_t AlphaBeta(const uint8_t ply, int16_t depth, int32_t alpha, int32_t beta)
        {
            //region Out of Time & Force Stop
            if (Stop || ((Nodes & 4095) == 0 && TC.Finished<true>())) [[unlikely]] { Stop = true; return Draw; }
            //endregion

            constexpr auto OColor = Opposite(Color);

            //region PV Table Ply Initialization
            PVTable[ply].Ply = ply;
            //endregion

            //region Selected Depth Change
            if (Pv) SelectiveDepth = std::max(SelectiveDepth, ply);
            //endregion

            //region Q Jump
            if (depth <= 0) return Q<Color, Pv>(ply, alpha, beta);
            //endregion

            //region Zobrist Hash
            const ZobristHash hash = Board.Zobrist();
            //endregion

            if (!Root) {
                //region Draw Detection
                if (Stack[ply].HalfMoveCounter >= 100) return Draw;

                if (Repetition.Found(hash, Stack[ply].HalfMoveCounter)) return Draw;

                const uint8_t pieceCount = Count(~Board[NAC]);

                if (pieceCount == 2) return Draw;

                const bool knightLeft = Board.PieceBoard<White>(Knight) | Board.PieceBoard<Black>(Knight),
                           bishopLeft = Board.PieceBoard<White>(Bishop) | Board.PieceBoard<Black>(Bishop);
                if (pieceCount == 3 && (knightLeft || bishopLeft)) return Draw;
                //endregion

                //region Mate Pruning
                alpha = std::max(alpha, -Mate + ply    );
                beta  = std::min(beta ,  Mate - ply - 1);
                if (alpha >= beta) return alpha;
                //endregion
            }

            //region Transposition Table Lookup
            const SearchState& ttState = TTable[hash];
            Move               ttMove  = NoMove;
            bool               ttHit   = false;

            if (ttState.Type != Invalid && ttState.Hash == hash) {
                ttHit  = true;
                ttMove = ttState.Move;

                if (!Pv && ttState.Depth >= depth) {
                    TTHits[ttState.Type]++;

                    if (ttState.Type == Exact                                        ) return ttState.Evaluation;
                    if (ttState.Type == BetaCutoff     && ttState.Evaluation >= beta ) return ttState.Evaluation;
                    if (ttState.Type == AlphaUnchanged && ttState.Evaluation <= alpha) return ttState.Evaluation;

                    TTHits[ttState.Type]--;
                }
            }
            //endregion

            //region Static Evaluation
            const int32_t staticEvaluation = ttHit ? StaticEvaluationTT<Color>(ttState) : Evaluation::Evaluate(Color);

            Stack[ply].StaticEvaluation = staticEvaluation;
            //endregion

            const bool checked   = Board.Checked<Color>();
            bool       improving = false;

            if (!Pv && !checked) {
                improving = ply >= 2 && staticEvaluation >= Stack[ply - 2].StaticEvaluation;

                //region Reverse Futility Pruning
                if (RFP(depth, staticEvaluation, improving, beta)) return beta;
                //endregion

                //region Razoring
                if (depth == 1 && staticEvaluation + RazoringEvaluationThreshold < alpha)
                    return Q<Color, false>(ply, alpha, beta);
                //endregion

                //region Null Move Pruning
                if (NMP<Color, Root>(ply, depth, staticEvaluation, beta)) return beta;
                //endregion
            } else if (checked) {
                //region Check Extension
                depth += CheckExtension;
                //endregion
            }

            //region IIR
            if (depth > IIRDepthThreshold && !ttHit) depth -= IIRDepthReduction;
            //endregion

            //region MoveList
            using MoveList = OrderedMoveList<Color>;
            MoveList moves (Board, ply, KTable, HTable, ttMove);
            //endregion

            //region Checkmate & Stalemate Detection
            if (moves.Count() == 0) return checked ? -Mate + ply : 0;
            //endregion

            //region Fail-soft Alpha Beta Negamax
            const uint8_t lmpQuietThreshold = LMPQuietThresholdBase + depth * depth;
            const bool    lmp               = !Root && !checked && depth <= LMPDepthThreshold;
            const bool    lmr               = depth >= LMRDepthThreshold && !checked;

            SearchState abState {
                hash,
                -Infinity,
                ttMove,
                static_cast<uint8_t>(depth),
                AlphaUnchanged
            };

            uint8_t quietMoveCount = 0;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move  = moves[i];
                const bool quiet = Board[move.To()].Piece() == NAP;
                quietMoveCount += quiet;

                //region Futility Pruning
                if (i > 0 && quiet && staticEvaluation + depth * FutilityDepthFactor <= alpha)
                    break;
                //endregion

                //region Late Move Pruning
                if (!Pv && lmp && abState.Evaluation > -Infinity && quietMoveCount > lmpQuietThreshold)
                    break;
                //endregion

                const PreviousState boardState = EngineMove<true>(move, ply, quiet);

                int32_t evaluation = 0;
                if (i == 0)
                    evaluation = -AlphaBeta<OColor, Pv, false>(ply + 1, depth - 1, -beta, -alpha);
                else {
                    //region Late Move Reduction
                    if (i >= LMRFullSearchThreshold && lmr) {
                        int16_t r = LogarithmicReductionTable[depth][i];

                        if (!Pv) r++;

                        if (!improving) r++;

                        if (Board.Checked<OColor>()) r--;

                        const int16_t reducedDepth = static_cast<int16_t>(std::max(depth - r, 1));

                        evaluation =
                            -AlphaBeta<OColor, false, false>(ply + 1, reducedDepth, -alpha - 1, -alpha);
                    } else evaluation = alpha + 1;
                    //endregion

                    //region Principal Variation Search
                    evaluation = PVS<Color>(evaluation, ply, depth, alpha, beta);
                    //endregion
                }

                EngineUndoMove<true>(boardState, move);

                //region Handle Evaluation
                if (evaluation <= abState.Evaluation) continue;

                abState.Evaluation = evaluation;

                if (evaluation <= alpha) continue;

                alpha = evaluation;

                abState.Type = Exact;
                abState.Move = move;

                // ReSharper disable once CppDFAConstantConditions
                if (Pv && !Stop) {
                    PVTable[ply].PV[ply] = move;

                    for (uint8_t nPly = ply + 1; nPly < PVTable[ply + 1].Ply; nPly++)
                        PVTable[ply].PV[nPly] = PVTable[ply + 1].PV[nPly];

                    PVTable[ply].Ply = PVTable[ply + 1].Ply;
                }

                if (evaluation < beta) continue;

                // ReSharper disable once CppDFAConstantConditions
                if (quiet && !Stop) {
                    if (KTable[0][ply] != move) {
                        KTable[1][ply] = KTable[0][ply];
                        KTable[0][ply] = move;
                    }

                    UpdateHistory<Color, true>(move, depth);

                    for (uint8_t q = 1; q < quietMoveCount; q++) {
                        const Move other = moves.UnsortedAccess(i - q);
                        UpdateHistory<Color, false>(other, depth);
                    }
                }

                abState.Type = BetaCutoff;
                break;
                //endregion
            }
            //endregion

            //region Transposition Table Insertion
            // ReSharper disable once CppDFAConstantConditions
            if (!Stop) InsertEntry(hash, abState);
            //endregion

            return abState.Evaluation;
        }

        template<Color Color, bool Pv>
        int32_t Q(const uint8_t ply, int32_t alpha, const int32_t beta)
        {
            constexpr auto OColor = Opposite(Color);

            //region Selective Depth Change
            if (Pv) SelectiveDepth = std::max(SelectiveDepth, ply);
            //endregion

            //region Transposition Table Lookup
            else {
                const ZobristHash  hash    = Board.Zobrist();
                const SearchState& ttState = TTable[hash];

                if (ttState.Hash == hash) {
                    if (ttState.Type == Exact                                        ) return ttState.Evaluation;
                    if (ttState.Type == BetaCutoff     && ttState.Evaluation >= beta ) return ttState.Evaluation;
                    if (ttState.Type == AlphaUnchanged && ttState.Evaluation <= alpha) return ttState.Evaluation;
                }
            }
            //endregion

            //region Static Evaluation
            const int32_t staticEvaluation = Evaluation::Evaluate(Color);

            if (staticEvaluation >= beta) return beta;
            if (staticEvaluation > alpha) alpha = staticEvaluation;
            //endregion

            //region MoveList
            using MoveList = OrderedMoveList<Color, true>;
            MoveList moves (Board, ply, KTable, HTable, NoMove);
            //endregion

            //region Fail-soft Alpha Beta Negamax
            int32_t bestEvaluation = staticEvaluation;
            for (uint8_t i = 0; i < moves.Count(); i++) {
                const Move move = moves[i];

                //region SEE Pruning
                if (!SEE::Accurate(Board, move, 0)) continue;
                //endregion

                const PreviousState state = EngineMove<false>(move, ply);

                const int32_t evaluation = -Q<OColor, Pv>(ply + 1, -beta, -alpha);

                EngineUndoMove<false>(state, move);

                //region Handle Evaluation
                if (evaluation <= bestEvaluation) continue;

                bestEvaluation = evaluation;

                if (evaluation <= alpha) continue;

                alpha = evaluation;

                if (evaluation >= beta) break;
                //endregion
            }
            //endregion

            return bestEvaluation;
        }

        template<Color Color, bool Root>
        bool NMP(const uint8_t ply, const int16_t depth, const int32_t staticEvaluation, const int32_t beta)
        {
            if (Root || depth < NullMoveDepth || staticEvaluation < beta) return false;

            constexpr auto OColor = Opposite(Color);

            const auto reductionStep   = static_cast<int16_t>(depth / NullMoveDepth);
            const auto reductionFactor = static_cast<int16_t>((staticEvaluation - beta) / NullMoveEvaluationMargin);
            const auto reduction       = static_cast<int16_t>(NullMoveDepth + reductionStep +
                                            std::min<int16_t>(NullMoveDepth, reductionFactor));

            const PreviousStateNull state = Board.Move();

            const int32_t evaluation = -AlphaBeta<OColor, false, false>
                    (ply + 1, depth - reduction, -beta, -beta + 1);

            Board.UndoMove(state);

            return evaluation >= beta;
        }

        template<Color Color>
        int32_t PVS(      int32_t evaluation, const uint8_t ply , const int16_t depth,
                    const int32_t alpha     , const int32_t beta)
        {
            if (evaluation <= alpha) return evaluation;

            constexpr auto OColor = Opposite(Color);

            evaluation = -AlphaBeta<OColor, false, false>(ply + 1, depth - 1, -alpha - 1, -alpha);

            if (evaluation <= alpha || evaluation >= beta) return evaluation;

            return -AlphaBeta<OColor, true, false>(ply + 1, depth - 1, -beta, -alpha);
        }

        template<bool UpdateHistory>
        PreviousState EngineMove(const Move move, const uint8_t ply, const bool quiet = false)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            if (!quiet || Board[move.From()].Piece() == Pawn)
                 Stack[ply + 1].HalfMoveCounter = 0;
            else Stack[ply + 1].HalfMoveCounter = Stack[ply].HalfMoveCounter + 1;

            const PreviousState state = Board.Move<MT>(move.From(), move.To(), move.Promotion());
            Nodes++;

            const ZobristHash hash = Board.Zobrist();

            TTable.Prefetch(hash);

            if (UpdateHistory) Repetition.Push(hash);

            return state;
        }

        template<bool UpdateHistory>
        void EngineUndoMove(const PreviousState state, const Move move)
        {
            constexpr MoveType MT = NNUE | ZOBRIST;

            Board.UndoMove<MT>(state, move.From(), move.To());

            if (UpdateHistory) Repetition.Pull();
        }

        template<Color Color, bool Increase>
        void UpdateHistory(const Move move, const int16_t depth)
        {
            const int16_t bonus = std::min<int16_t>(300 * depth - 250, HistoryLimit);

            int16_t& history = HTable[Color][Board[move.From()].Piece()][move.To()];

            history += bonus * (Increase ? 1 : -1) - history * bonus / HistoryLimit;
        }

        static bool RFP(const int16_t depth, const int32_t     staticEvaluation,
                        const bool    improving, const int32_t beta)
        {
            return depth < ReverseFutilityDepthThreshold &&
                   abs(beta) < Mate &&
                   staticEvaluation - ReverseFutilityD * depth + improving * ReverseFutilityI >= beta;
        }

        static void InsertEntry(const ZobristHash hash, const SearchState& entry)
        {
            // ReSharper disable once CppTooWideScopeInitStatement
            const SearchState& old = TTable[hash];
            if (entry.Type == Exact || entry.Hash != old.Hash ||
               (old.  Type == AlphaUnchanged &&
                entry.Type == BetaCutoff   ) ||
                entry.Depth > old.Depth - ReplacementThreshold)
                TTable[hash] = entry;
        }

        template<Color Color>
        static int32_t StaticEvaluationTT(const SearchState& entry)
        {
            if (entry.Type == Exact) return entry.Evaluation;

            // ReSharper disable once CppTooWideScopeInitStatement
            const int32_t staticEvaluation = Evaluation::Evaluate(Color);

            if ((staticEvaluation > entry.Evaluation && entry.Type == BetaCutoff    )  ||
                (staticEvaluation < entry.Evaluation && entry.Type == AlphaUnchanged)) return staticEvaluation;

            return entry.Evaluation;
        }

    };

} // StockDory

#endif //STOCKDORY_SEARCH_H
