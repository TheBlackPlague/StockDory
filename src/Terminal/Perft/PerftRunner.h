//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PERFTRUNNER_H
#define STOCKDORY_PERFTRUNNER_H

#include <iostream>
#include <chrono>
#include <algorithm>
#include <execution>

#include "../../Backend/Board.h"
#include "../../Backend/TranspositionTable.h"
#include "../../Backend/Util.h"
#include "../../Backend/Move/MoveList.h"

#include "PerftEntry.h"

namespace StockDory::Perft
{

    class PerftRunner
    {

        private:
            static Board PerftBoard;
            static TranspositionTable<PerftEntry<5>> TranspositionTable;

            template<::Color Color, bool Divide, bool Sync = false, bool TT = false>
            struct PerftLayer
            {

                public:
                    static inline uint64_t Perft(Board& board, const uint8_t depth)
                    {
                        return PerftRunner::Perft<Color, Divide, Sync, TT>(board, depth);
                    }

                    template<Piece Piece>
                    static inline uint64_t PerftLoop(Board& board, const uint8_t depth,
                                                     const PinBitBoard& pin, const CheckBitBoard& check,
                                                     const BitBoardIterator& iterator)
                    {
                        return PerftRunner::PerftLoop<Piece, Color, Divide, Sync, TT>
                                (board, depth, pin, check, iterator);
                    }

            };

            template<MoveType T>
            struct BoardLayer
            {

                public:
                    static inline PreviousState Move(Board& board,
                                                     const Square from, const Square to,
                                                     const Piece promotion = NAP)
                    {
                        return board.Move<T>(from, to, promotion);
                    }

                    static inline void UndoMove(Board& board, const PreviousState& state,
                                                const Square from, const Square to)
                    {
                        board.UndoMove<T>(state, from, to);
                    }

            };

            template<::Color Color, bool Divide, bool Sync = false, bool TT = false>
            static inline uint64_t Perft(Board& board, const uint8_t depth)
            {
                uint64_t nodes = 0;
                using PLayer = PerftLayer<Color, Divide, Sync, TT>;

                const PinBitBoard   pin   = board.Pin  <Color, Opposite(Color)>();
                const CheckBitBoard check = board.Check<       Opposite(Color)>();

                if (check.DoubleCheck) {
                    const BitBoardIterator kings (board.PieceBoard<Color>(King));
                    nodes += PLayer::template PerftLoop<King>(board, depth, pin, check, kings);
                    return nodes;
                }

                const BitBoardIterator pawns   (board.PieceBoard<Color>(Pawn  ));
                const BitBoardIterator knights (board.PieceBoard<Color>(Knight));
                const BitBoardIterator bishops (board.PieceBoard<Color>(Bishop));
                const BitBoardIterator rooks   (board.PieceBoard<Color>(Rook  ));
                const BitBoardIterator queens  (board.PieceBoard<Color>(Queen ));
                const BitBoardIterator kings   (board.PieceBoard<Color>(King  ));

                nodes += PLayer::template PerftLoop<Pawn  >(board, depth, pin, check, pawns  );
                nodes += PLayer::template PerftLoop<Knight>(board, depth, pin, check, knights);
                nodes += PLayer::template PerftLoop<Bishop>(board, depth, pin, check, bishops);
                nodes += PLayer::template PerftLoop<Rook  >(board, depth, pin, check, rooks  );
                nodes += PLayer::template PerftLoop<Queen >(board, depth, pin, check, queens );
                nodes += PLayer::template PerftLoop<King  >(board, depth, pin, check, kings  );

                return nodes;
            }

            template<Piece Piece, ::Color Color, bool Divide, bool Sync = false, bool TT = false>
            static inline uint64_t PerftLoop(Board& board, const uint8_t depth,
                                             const PinBitBoard& pin, const CheckBitBoard& check,
                                             BitBoardIterator pIterator)
            {
                uint64_t nodes = 0;
                using PLayer = PerftLayer<Opposite(Color), false, Sync, TT>;
                using BLayer = BoardLayer<TT ? (PERFT | ZOBRIST) : STANDARD>;

                if (TT) {
                    const ZobristHash hash = board.Zobrist();
                    PerftEntry<5>& entry = TranspositionTable[hash];

                    const std::pair<bool, uint64_t> entryResult = entry.Nodes(hash, depth);
                    if (entryResult.first) return entryResult.second;
                }

                if (depth == 1) for (Square sq = pIterator.Value(); sq != Square::NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (board, sq, pin, check);
                    uint8_t count = moves.Count();

                    if (moves.Promotion(sq)) nodes += count * 4;
                    else                     nodes += count    ;

                    if (Divide && count) {
                        BitBoardIterator mIterator = moves.Iterator();

                        for (Square m = mIterator.Value(); m != Square::NASQ; m = mIterator.Value()) {
                            if (moves.Promotion(sq)) {
                                 LogMove<Queen >(sq, m, 1);
                                 LogMove<Rook  >(sq, m, 1);
                                 LogMove<Bishop>(sq, m, 1);
                                 LogMove<Knight>(sq, m, 1);
                            }
                            else LogMove        (sq, m, 1);
                        }
                    }
                } else if (Sync || depth < 5)
                for (Square sq = pIterator.Value(); sq != Square::NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (board, sq, pin, check);

                    BitBoardIterator mIterator = moves.Iterator();

                    for (Square m = mIterator.Value(); m != Square::NASQ; m = mIterator.Value()) {
                        if (moves.Promotion(sq)) {
                            PreviousState state = BLayer::Move(board, sq, m, Queen);
                            const uint64_t queenNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += queenNodes;

                            if (Divide) LogMove<Queen >(sq, m, queenNodes);

                            state = BLayer::Move(board, sq, m, Rook);
                            const uint64_t rookNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += rookNodes;

                            if (Divide) LogMove<Rook  >(sq, m, rookNodes);

                            state = BLayer::Move(board, sq, m, Bishop);
                            const uint64_t bishopNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += bishopNodes;

                            if (Divide) LogMove<Bishop>(sq, m, bishopNodes);

                            state = BLayer::Move(board, sq, m, Knight);
                            const uint64_t knightNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += knightNodes;

                            if (Divide) LogMove<Knight>(sq, m, knightNodes);
                        } else {
                            const PreviousState state = BLayer::Move(board, sq, m);
                            const uint64_t perftNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += perftNodes;

                            if (Divide) LogMove(sq, m, perftNodes);
                        }
                    }
                } else {
                    std::atomic<uint64_t> atomicNodes;
                    std::vector<Square> psq = pIterator.Values();
                    std::for_each(std::execution::par, psq.begin(), psq.end(), [&](const Square sq) {
                        Board parallelBoard = board;

                        MoveList<Piece, Color> moves (parallelBoard, sq, pin, check);
                        if (moves.Count() < 0) return;

                        uint64_t parallelNodes = 0        ;
                        uint8_t  nextDepth     = depth - 1;

                        BitBoardIterator mIterator = moves.Iterator();

                        for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                            if (moves.Promotion(sq)) {
                                PreviousState state = BLayer::Move(parallelBoard, sq, m, Queen);
                                const uint64_t queenNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += queenNodes;

                                if (Divide) LogMove<Queen >(sq, m, queenNodes);

                                state = BLayer::Move(parallelBoard, sq, m, Rook);
                                const uint64_t rookNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += rookNodes;

                                if (Divide) LogMove<Rook  >(sq, m, rookNodes);

                                state = BLayer::Move(parallelBoard, sq, m, Bishop);
                                const uint64_t bishopNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += bishopNodes;

                                if (Divide) LogMove<Bishop>(sq, m, bishopNodes);

                                state = BLayer::Move(parallelBoard, sq, m, Knight);
                                const uint64_t knightNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += knightNodes;

                                if (Divide) LogMove<Knight>(sq, m, knightNodes);
                            } else {
                                const PreviousState state = BLayer::Move(parallelBoard, sq, m);
                                const uint64_t perftNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += perftNodes;

                                if (Divide) LogMove(sq, m, perftNodes);
                            }
                        }

                        atomicNodes += parallelNodes;
                    });

                    nodes += atomicNodes;
                }

                if (TT) {
                    const ZobristHash hash = board.Zobrist();
                    PerftEntry<5>& entry = TranspositionTable[hash];
                    entry.Insert(hash, depth, nodes);
                }

                return nodes;
            }

            template<Piece Promotion = NAP>
            static void LogMove(const Square from, const Square to, const uint64_t nodes)
            {
                std::string logEntry = Util::SquareToString(from) + Util::SquareToString(to);
                if (Promotion != NAP) logEntry += static_cast<char>(tolower(FirstLetter(Promotion)));
                logEntry += ": " + std::to_string(nodes) + "\n";
                std::cout << logEntry;
            }

        public:
            static void SetBoard(const std::string& fen)
            {
                PerftBoard = Board(fen);
            }

            static void SetBoard(const Board& board)
            {
                PerftBoard = board;
            }

            static void SetTranspositionTable(const uint64_t size)
            {
                TranspositionTable = StockDory::TranspositionTable<PerftEntry<5>>(size);
            }

            template <bool Divide, bool TT = false>
            static void Perft(const uint8_t depth)
            {
                std::cout << "Running PERFT @ depth " << static_cast<uint32_t>(depth) << ":" << std::endl;

                auto start = std::chrono::high_resolution_clock::now();
                const uint64_t nodes =
                        PerftBoard.ColorToMove() == White ?
                        Perft<White, Divide, false, TT>(PerftBoard, depth) :
                        Perft<Black, Divide, false, TT>(PerftBoard, depth);
                auto stop  = std::chrono::high_resolution_clock::now();
                auto time = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

                std::cout << "Searched " << nodes << " nodes. (" << time << "µs)" << std::endl;
            }

    };

} // Perft

StockDory::Board StockDory::Perft::PerftRunner::PerftBoard = StockDory::Board();
StockDory::TranspositionTable<StockDory::Perft::PerftEntry<5>>
        StockDory::Perft::PerftRunner::TranspositionTable =
        StockDory::TranspositionTable<StockDory::Perft::PerftEntry<5>>(0);

#endif //STOCKDORY_PERFTRUNNER_H
