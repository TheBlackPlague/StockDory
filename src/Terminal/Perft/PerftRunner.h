//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PERFTRUNNER_H
#define STOCKDORY_PERFTRUNNER_H

#include <chrono>
#include <iostream>

#include "../../Backend/Board.h"
#include "../../Backend/ThreadPool.h"
#include "../../Backend/Util.h"
#include "../../Backend/Move/MoveList.h"

#include "PerftEntry.h"

// using PEntry = StockDory::Perft::PerftEntry<9>;

namespace StockDory
{

    class PerftRunner
    {

        static Board PerftBoard;
        // static TranspositionTable<PEntry> TranspositionTable;

        template<Color Color, bool Divide, bool Sync = false, bool TT = false>
        struct PerftLayer
        {

            static inline uint64_t Perft(Board& board, const uint8_t depth)
            {
                return PerftRunner::Perft<Color, Divide, Sync, TT>(board, depth);
            }

            template<Piece Piece>
            static inline uint64_t PerftLoop(            Board&      board, const uint8_t        depth,
                                             const PinBitBoard&      pin,   const CheckBitBoard& check,
                                             const BitBoardIterator& iterator)
            {
                return PerftRunner::PerftLoop<Piece, Color, Divide, Sync, TT>(board, depth, pin, check, iterator);
            }

        };

        template<MoveType T>
        struct BoardLayer
        {

            static inline PreviousState Move(Board&       board,
                                             const Square from, const Square to,
                                             const Piece  promotion = NAP)
            {
                return board.Move<T>(from, to, promotion);
            }

            static inline void UndoMove(Board&       board, const PreviousState& state,
                                        const Square from, const Square          to)
            {
                board.UndoMove<T>(state, from, to);
            }

        };

        template<Color Color, bool Divide, bool Sync = false, bool TT = false>
        static inline uint64_t Perft(Board& board, const uint8_t depth)
        {
            uint64_t nodes = 0;
            using PLayer   = PerftLayer<Color, Divide, Sync, TT>;

            // if (TT) {
            //     const ZobristHash hash = board.Zobrist();
            //     PEntry& entry = TranspositionTable[hash];
            //     std::pair<bool, uint64_t> result = entry.Nodes(hash, depth);
            //
            //     if (result.first) return result.second;
            // }

            const PinBitBoard   pin   = board.Pin<Color, Opposite(Color)>();

            if (const CheckBitBoard    check = board.Check<Opposite(Color)>(); check.DoubleCheck) {
                const BitBoardIterator kings   (board.PieceBoard<Color>(King  ));
                nodes += PLayer::template PerftLoop<King>(board, depth, pin, check, kings);
            } else {
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
            }

            // if (TT) {
            //     const ZobristHash hash = board.Zobrist();
            //     PEntry& entry = TranspositionTable[hash];
            //     entry.Insert(hash, depth, nodes);
            // }

            return nodes;
        }

        template<Piece Piece, Color Color, bool Divide, bool Sync = false, bool TT = false>
        static inline uint64_t PerftLoop(Board&             board, const uint8_t      depth,
                                         const PinBitBoard& pin, const CheckBitBoard& check,
                                         BitBoardIterator   pIterator)
        {
            uint64_t nodes = 0;
            using PLayer   = PerftLayer<Opposite(Color), false, Sync, TT>;
            using BLayer   = BoardLayer<TT ? PERFT | ZOBRIST : STANDARD>;

            if (depth == 1)
                for (Square sq = pIterator.Value(); sq != NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (board, sq, pin, check);
                    const uint8_t        count = moves.Count();

                    if (moves.Promotion(sq)) nodes += count * 4;
                    else                     nodes += count;

                    if (Divide && count) {
                        BitBoardIterator mIterator = moves.Iterator();

                        for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                            if (moves.Promotion(sq)) {
                                LogMove<Queen >(sq, m, 1);
                                LogMove<Rook  >(sq, m, 1);
                                LogMove<Bishop>(sq, m, 1);
                                LogMove<Knight>(sq, m, 1);
                            } else LogMove(sq, m, 1);
                        }
                    }
                }
            else if (Sync || depth < 5)
                for (Square sq = pIterator.Value(); sq != NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (board, sq, pin, check);

                    BitBoardIterator mIterator = moves.Iterator();

                    for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                        if (moves.Promotion(sq)) {
                            PreviousState  state       = BLayer::Move (board, sq, m, Queen );
                            const uint64_t queenNodes  = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += queenNodes;

                            if (Divide) LogMove<Queen >(sq, m,  queenNodes);

                            state                      = BLayer::Move (board, sq, m, Rook  );
                            const uint64_t rookNodes   = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += rookNodes;

                            if (Divide) LogMove<Rook  >(sq, m,   rookNodes);

                            state                      = BLayer::Move (board, sq, m, Bishop);
                            const uint64_t bishopNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += bishopNodes;

                            if (Divide) LogMove<Bishop>(sq, m, bishopNodes);

                            state                      = BLayer::Move (board, sq, m, Knight);
                            const uint64_t knightNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += knightNodes;

                            if (Divide) LogMove<Knight>(sq, m, knightNodes);
                        } else {
                            const PreviousState state      = BLayer::Move (board, sq, m);
                            const uint64_t      perftNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                            nodes += perftNodes;

                            if (Divide) LogMove(sq, m, perftNodes);
                        }
                    }
                }
            else {
                std::array<Square, 64>                psq     = {};
                std::array<std::future<uint64_t>, 64> futures = {};
                const uint8_t                         count   = pIterator.ToArray(psq);

                BS::blocks<size_t> blocks(0, count, std::thread::hardware_concurrency());

                auto ParallelComputation = [depth, &board, &pin, &check, &psq, &blocks](const size_t b) -> uint64_t
                {
                    const uint8_t start = blocks.start(b);
                    const uint8_t end   = blocks.  end(b);

                    uint64_t parallelNodes = 0;
                    uint8_t  nextDepth     = depth - 1;

                    Board parallelBoard = board;

                    for (uint8_t i = start; i < end; i++) {
                        const Square sq = psq[i];

                        MoveList<Piece, Color> moves (parallelBoard, sq, pin, check);
                        if (moves.Count() < 1) return 0;

                        BitBoardIterator mIterator = moves.Iterator();

                        for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                            if (moves.Promotion(sq)) {
                                PreviousState  state       = BLayer::Move (parallelBoard, sq, m, Queen );
                                const uint64_t queenNodes  = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += queenNodes;

                                if (Divide) LogMove<Queen>(sq, m, queenNodes);

                                state                      = BLayer::Move (parallelBoard, sq, m, Rook  );
                                const uint64_t rookNodes   = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += rookNodes;

                                if (Divide) LogMove<Rook>(sq, m, rookNodes);

                                state                      = BLayer::Move (parallelBoard, sq, m, Bishop);
                                const uint64_t bishopNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += bishopNodes;

                                if (Divide) LogMove<Bishop>(sq, m, bishopNodes);

                                state                      = BLayer::Move (parallelBoard, sq, m, Knight);
                                const uint64_t knightNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += knightNodes;

                                if (Divide) LogMove<Knight>(sq, m, knightNodes);
                            } else {
                                const PreviousState state      = BLayer::Move (parallelBoard, sq, m);
                                const uint64_t      perftNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += perftNodes;

                                if (Divide) LogMove(sq, m, perftNodes);
                            }
                        }
                    }

                    return parallelNodes;
                };

                size_t b = 0;
                while (b < blocks.get_num_blocks()) {
                    futures[b] = std::async(std::launch::async, ParallelComputation, b);
                    b++;
                }

                for (size_t f = 0; f < b; f++) nodes += futures[f].get();
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

        // static void SetTranspositionTable(const uint64_t bytes)
        // {
        //     std::cout << "Allocating table using defined bytes (" << bytes << ")\n";
        //     TranspositionTable = StockDory::TranspositionTable<PEntry>(bytes);
        //     std::cout << "Table: " << TranspositionTable.Size() << " entries\n";
        //     std::cout << "Table: " << TranspositionTable.Size() * sizeof(PEntry) << " bytes\n";
        // }

        template<bool Divide, bool TT = false>
        static void Perft(const uint8_t depth)
        {
            std::cout << "Running PERFT @ depth " << static_cast<uint32_t>(depth) << ":" << std::endl;

            const auto     start = std::chrono::high_resolution_clock::now();
            const uint64_t nodes =
                PerftBoard.ColorToMove() == White
                ? Perft<White, Divide, false, TT>(PerftBoard, depth)
                : Perft<Black, Divide, false, TT>(PerftBoard, depth);
            const auto     stop  = std::chrono::high_resolution_clock::now();
            const auto     time  = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

            std::cout << "Searched " << nodes << " nodes. (" << time << "µs)" << std::endl;
        }

    };

} // Perft

StockDory::Board StockDory::PerftRunner::PerftBoard = Board();
// StockDory::TranspositionTable<PEntry> StockDory::Perft::PerftRunner::TranspositionTable =
// StockDory::TranspositionTable<PEntry>(0);

#endif //STOCKDORY_PERFTRUNNER_H
