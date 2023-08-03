//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SEARCH_H
#define STOCKDORY_SEARCH_H

#include "../Backend/Board.h"
#include "../Backend/Type/Move.h"
#include "../Backend/Type/Zobrist.h"

#include "Move/OrderedMoveList.h"
#include "Move/PrincipleVariationTable.h"
#include "Move/KillerTable.h"
#include "Move/HistoryTable.h"
#include "Time/TimeManager.h"
#include "Evaluation.h"
#include "EngineParameter.h"
#include "LogarithmicReductionTable.h"
#include "SEE.h"
#include "RepetitionHistory.h"

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

            [[nodiscard]]
            inline bool BeyondLimit(const uint64_t nodes, const uint8_t depth) const
            {
                return nodes > Nodes || depth > Depth;
            }

    };

    class NoLogger
    {

        public:
            static void LogDepthIteration([[maybe_unused]] const uint8_t depth,
                                          [[maybe_unused]] const uint8_t selectiveDepth,
                                          [[maybe_unused]] const int32_t evaluation,
                                          [[maybe_unused]] const uint64_t nodes,
                                          [[maybe_unused]] const uint64_t ttNodes,
                                          [[maybe_unused]] const MS time,
                                          [[maybe_unused]] const std::string& pv) {}

            static void LogBestMove([[maybe_unused]] const Move& move) {}

    };

    template<class Logger = NoLogger>
    class Search
    {

        private:
            class SearchStopException : public std::exception {};

            struct SearchStackEntry
            {

                int32_t StaticEvaluation = 0;
                uint8_t HalfMoveCounter  = 0;

            };

            Board       Board;
            TimeControl TC   ;

            RepetitionHistory Repetition = RepetitionHistory(0);

            PrincipleVariationTable PvTable = PrincipleVariationTable();
                        KillerTable  KTable = KillerTable            ();
                       HistoryTable  HTable = HistoryTable           ();

            std::array<SearchStackEntry, MaxDepth> Stack = {};

            uint8_t SelectiveDepth = 0;

            uint64_t   Nodes = 0;
            uint64_t TTNodes = 0;

            int32_t Evaluation = -Infinity;
            Move    BestMove   = NoMove   ;

            uint8_t BestMoveStability = 0;

            bool Stop = false;

        public:
            Search() = default;

            Search(const StockDory::Board  board,      const StockDory::TimeControl tc,
                   const RepetitionHistory repetition, const uint8_t                hm)
                   : Board(board), TC(tc), Repetition(repetition)
            {
                Stack[0].HalfMoveCounter = hm;
            }

            void IterativeDeepening(const Limit limit)
            {
                Board.LoadForEvaluation();

                TC.Start();

                try {
                    int16_t currentDepth = 1;
                    while (!limit.BeyondLimit(Nodes, currentDepth) && !TC.Finished<false>()) {
                        const Move lastBestMove = BestMove;

                        if (Board.ColorToMove() == White)
                             Evaluation = Aspiration<White>(currentDepth);
                        else Evaluation = Aspiration<Black>(currentDepth);

                        BestMove = PvTable[0];

                        BestMoveStabilityOptimisation(lastBestMove);

                        Logger::LogDepthIteration(currentDepth, SelectiveDepth,
                                                  Evaluation,
                                                  Nodes, TTNodes,
                                                  TC.Elapsed(), PvLine());
                        currentDepth++;
                    }
                } catch (SearchStopException&) {}

                Logger::LogBestMove(BestMove);
            }

            [[nodiscard]]
            inline uint64_t NodesSearched() const
            {
                return Nodes;
            }

            [[nodiscard]]
            inline std::string PvLine() const
            {
                std::stringstream line;
                uint8_t ply = PvTable.Count();

                for (uint8_t i = 0; i < ply; i++) {
                    line << PvTable[i].ToString();
                    if (i != ply - 1) line << " ";
                }

                return line.str();
            }

            inline void ForceStop()
            {
                Stop = true;
            }

        private:
            void BestMoveStabilityOptimisation(const Move lastBestMove)
            {
                if (lastBestMove == BestMove) BestMoveStability = std::min(BestMoveStability + 1, 4);
                else                          BestMoveStability = 0;

                TimeManager::Optimise(TC, BestMoveStabilityOptimisationFactor[BestMoveStability]);
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
                    if (Stop || TC.Finished<true>()) throw SearchStopException();
                    //endregion

                    //region Reset Window
                    if (alpha < -AspirationBound) alpha = -Infinity;
                    if (beta  >  AspirationBound) beta  =  Infinity;
                    //endregion

                    int32_t bestEvaluation = AlphaBeta<Color, true, true>(0, depth, alpha, beta);

                    //region Modify Window
                    if        (bestEvaluation <= alpha) {
                        research++;

                        alpha = std::max(alpha - research * research * AspirationDelta, -Infinity);
                    } else if (bestEvaluation >= beta ) {
                        research++;

                        beta  = std::min(beta  + research * research * AspirationDelta,  Infinity);

                        BestMove = PvTable[0];
                    } else return bestEvaluation;
                    //endregion
                }
            }

            template<Color Color, bool Pv, bool Root>
            int32_t AlphaBeta(const uint8_t ply, int16_t depth, int32_t alpha, int32_t beta)
            {
                //region Out of Time & Force Stop
                if (Stop || ((Nodes & 4095) == 0 && TC.Finished<true>())) throw SearchStopException();
                //endregion

                constexpr enum Color OColor = Opposite(Color);

                //region PV Table Ply Initialization
                PvTable.InitializePly(ply);
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

                //region Mate Pruning & Draw Detection
                if (!Root) {
                    if (Stack[ply].HalfMoveCounter >= 100) return Draw;

                    if (Repetition.Found(hash, Stack[ply].HalfMoveCounter)) return Draw;

                    const uint8_t pieceCount = Count(~Board[NAC]);

                    if (pieceCount == 2) return Draw;

                    const bool knightLeft = Board.PieceBoard<White>(Knight) | Board.PieceBoard<Black>(Knight);
                    const bool bishopLeft = Board.PieceBoard<White>(Bishop) | Board.PieceBoard<Black>(Bishop);
                    if (pieceCount == 3 && (knightLeft || bishopLeft)) return Draw;

                    alpha = std::max(alpha, -Mate + ply    );
                    beta  = std::min(beta ,  Mate - ply - 1);
                    if (alpha >= beta) return alpha;
                }
                //endregion

                //region Transposition Table Lookup
                const EngineEntry& storedEntry = TTable[hash];
                Move ttMove = NoMove;
                bool ttHit  = false ;

                if (storedEntry.Type != Invalid && storedEntry.Hash == hash) {
                    ttHit  = true            ;
                    ttMove = storedEntry.Move;

                    if (!Pv && storedEntry.Depth >= depth) {
                        if (storedEntry.Type == Exact          ||
                           (storedEntry.Type == BetaCutoff     && storedEntry.Evaluation >= beta ) ||
                           (storedEntry.Type == AlphaUnchanged && storedEntry.Evaluation <= alpha)) {
                            TTNodes++;
                            return storedEntry.Evaluation;
                        }
                    }
                }
                //endregion

                //region Static Evaluation
                const int32_t staticEvaluation = ttHit ?
                        StaticEvaluationTT<Color>(storedEntry) : Evaluation::Evaluate<Color>();
                Stack[ply].StaticEvaluation = staticEvaluation;
                //endregion

                const bool checked = Board.Checked<Color>();
                bool improving = false;

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
                using MoveList = StockDory::OrderedMoveList<Color>;
                MoveList moves (Board, ply, KTable, HTable, ttMove);
                //endregion

                //region Checkmate & Stalemate Detection
                if (moves.Count() == 0) return checked ? -Mate + ply : 0;
                //endregion

                //region Fail-soft Alpha Beta Negamax
                int32_t         bestEvaluation = -Infinity;
                Move            bestMove       = NoMove;
                EngineEntryType ttEntryType    = AlphaUnchanged;

                const uint8_t lmpQuietThreshold = LMPQuietThresholdBase + depth * depth;
                const bool    lmp               = !Root && !checked && depth <= LMPDepthThreshold;
                const bool    lmr               = depth >= LMRDepthThreshold && !checked;
                const int32_t historyBonus      = depth * depth;
                const uint8_t historyFactor     = std::max(depth / 3, 1);

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
                    if (!Pv && lmp && bestEvaluation > -Infinity && quietMoveCount > lmpQuietThreshold)
                        break;
                    //endregion

                    const PreviousState state = EngineMove<true>(move, ply, quiet);

                    int32_t evaluation = 0;
                    if (i == 0) evaluation = -AlphaBeta<OColor, Pv, false>
                            (ply + 1, depth - 1, -beta, -alpha);
                    else {
                        //region Late Move Reduction
                        if (i >= LMRFullSearchThreshold && lmr) {
                            int16_t r = LogarithmicReductionTable::Get(depth, i);

                            if (!Pv) r++;

                            if (!improving) r++;

                            if (Board.Checked<OColor>()) r--;

                            const int16_t reducedDepth = static_cast<int16_t>(std::max(depth - r, 1));

                            evaluation = -AlphaBeta<OColor, false, false>
                                    (ply + 1, reducedDepth, -alpha - 1, -alpha);
                        } else evaluation = alpha + 1;
                        //endregion

                        //region Principal Variation Search
                        evaluation = PVS<Color>(evaluation, ply, depth, alpha, beta);
                        //endregion
                    }

                    EngineUndoMove<true>(state, move);

                    //region Handle Evaluation
                    if (evaluation <= bestEvaluation) continue;

                    bestEvaluation = evaluation;

                    if (evaluation <= alpha         ) continue;

                    alpha       = evaluation;
                    bestMove    = move      ;
                    ttEntryType = Exact     ;

                    if (Pv) {
                        PvTable.Insert(ply, move);

                        for (uint8_t nPly = ply + 1; PvTable.PlyInitialized(ply, nPly); nPly++)
                            PvTable.Copy(ply, nPly);

                        PvTable.Update(ply);
                    }

                    if (evaluation < beta           ) continue;

                    if (quiet) {
                        if (KTable.Get<1>(ply) != move) {
                            KTable.Reorder(ply);
                            KTable.Set<1>(ply, move);
                        }

                        HTable.Get(Board[move.From()].Piece(), Color, move.To())
                        += historyBonus + i * historyFactor;

                        for (uint8_t q = 1; q < quietMoveCount; q++) {
                            const Move other = moves.UnsortedAccess(i - q);
                            HTable.Get(Board[other.From()].Piece(), Color, other.To())
                            -= historyBonus + (quietMoveCount - q) * historyFactor;
                        }
                    }

                    ttEntryType = BetaCutoff;
                    //endregion
                    break;
                }
                //endregion

                //region Transposition Table Insertion
                auto entry = EngineEntry {
                    .Hash       = hash,
                    .Evaluation = bestEvaluation,
                    .Move       = ttEntryType != AlphaUnchanged ? bestMove : ttMove,
                    .Depth      = static_cast<uint8_t>(depth),
                    .Type       = ttEntryType
                };
                InsertEntry(hash, entry);
                //endregion

                return bestEvaluation;
            }

            template<Color Color, bool Pv>
            int32_t Q(const uint8_t ply, int32_t alpha, int32_t beta)
            {
                constexpr enum Color OColor = Opposite(Color);

                //region Selective Depth Change
                if (Pv) SelectiveDepth = std::max(SelectiveDepth, ply);
                //endregion

                //region Transposition Table Lookup
                if (!Pv) {
                    const ZobristHash  hash  = Board.Zobrist();
                    const EngineEntry& entry = TTable[hash];

                    if (entry.Hash == hash           &&
                       (entry.Type == Exact          ||
                       (entry.Type == BetaCutoff     && entry.Evaluation >= beta ) ||
                       (entry.Type == AlphaUnchanged && entry.Evaluation <= alpha)))
                        return entry.Evaluation;
                }
                //endregion

                //region Static Evaluation
                const int32_t staticEvaluation = Evaluation::Evaluate<Color>();

                if (staticEvaluation >=  beta) return beta;
                if (staticEvaluation >  alpha) alpha = staticEvaluation;
                //endregion

                //region MoveList
                using MoveList = StockDory::OrderedMoveList<Color, true>;
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

                    int32_t evaluation = -Q<OColor, Pv>(ply + 1, -beta, -alpha);

                    EngineUndoMove<false>(state, move);

                    //region Handle Evaluation
                    if (evaluation <= bestEvaluation) continue;
                    bestEvaluation = evaluation;

                    if (evaluation <= alpha         ) continue;
                    alpha          = evaluation;

                    if (evaluation >= beta          ) break;
                    //endregion
                }
                //endregion

                return bestEvaluation;
            }

            template<Color Color, bool Root>
            inline bool NMP(const uint8_t ply, const int16_t depth, const int32_t staticEvaluation, const int32_t beta)
            {
                if (Root || depth < NullMoveDepth || staticEvaluation < beta) return false;

                constexpr enum Color OColor = Opposite(Color);

                const auto reductionStep   = static_cast<int16_t>(depth / NullMoveDepth);
                const auto reductionFactor = static_cast<int16_t>((staticEvaluation - beta) / NullMoveEvaluationMargin);
                const auto reduction       = static_cast<int16_t>(NullMoveDepth + reductionStep +
                                                std::min<int16_t>(NullMoveDepth, reductionFactor));

                PreviousStateNull state = Board.Move();

                const int32_t evaluation = -AlphaBeta<OColor, false, false>
                        (ply + 1, depth - reduction, -beta, -beta + 1);

                Board.UndoMove(state);

                return evaluation >= beta;
            }

            template<Color Color>
            inline int32_t PVS(int32_t evaluation, const uint8_t ply, const int16_t depth,
                               const int32_t alpha, const int32_t beta)
            {
                if (evaluation <= alpha) return evaluation;

                constexpr enum Color OColor = Opposite(Color);

                evaluation =
                        -AlphaBeta<OColor, false, false>(ply + 1, depth - 1, -alpha - 1, -alpha);

                if (evaluation <= alpha || evaluation >= beta) return evaluation;

                return  -AlphaBeta<OColor,  true, false>(ply + 1, depth - 1, -beta     , -alpha);
            }

            template<bool UpdateHistory>
            inline PreviousState EngineMove(const Move move, const uint8_t ply, const bool quiet = false)
            {
                constexpr MoveType MT = NNUE | ZOBRIST;

                if (!quiet || Board[move.From()].Piece() == Pawn)
                     Stack[ply + 1].HalfMoveCounter =                              0;
                else Stack[ply + 1].HalfMoveCounter = Stack[ply].HalfMoveCounter + 1;

                const PreviousState state = Board.Move<MT>(move.From(), move.To(), move.Promotion());
                Nodes++;

                const ZobristHash hash = Board.Zobrist();

                TTable.Prefetch(hash);

                if (UpdateHistory) Repetition.Push(hash);

                return state;
            }

            template<bool UpdateHistory>
            inline void EngineUndoMove(const PreviousState state, const Move move)
            {
                constexpr MoveType MT = NNUE | ZOBRIST;

                Board.UndoMove<MT>(state, move.From(), move.To());

                if (UpdateHistory) Repetition.Pull();
            }

            static inline bool RFP(const int16_t depth, const int32_t staticEvaluation,
                                   const bool improving, const int32_t beta)
            {
                return depth        < ReverseFutilityDepthThreshold &&
                       abs(beta) < Mate                          &&
                       staticEvaluation - ReverseFutilityD * depth + improving * ReverseFutilityI >= beta;
            }

            static inline void InsertEntry(const ZobristHash hash, const EngineEntry& entry)
            {
                const EngineEntry& old = TTable[hash];
                if (entry.Type == Exact || entry.Hash != old.Hash ||
                   (old  .Type == AlphaUnchanged  &&
                    entry.Type == BetaCutoff    ) ||
                    entry.Depth > old.Depth - ReplacementThreshold)
                    TTable[hash] = entry;
            }

            template<Color Color>
            static inline int32_t StaticEvaluationTT(const EngineEntry& entry)
            {
                if (entry.Type == Exact) return entry.Evaluation;

                const int32_t staticEvaluation = Evaluation::Evaluate<Color>();

                if ((staticEvaluation > entry.Evaluation && entry.Type == BetaCutoff    ) ||
                    (staticEvaluation < entry.Evaluation && entry.Type == AlphaUnchanged)) return staticEvaluation;

                return entry.Evaluation;
            }

    };

} // StockDory

#endif //STOCKDORY_SEARCH_H
