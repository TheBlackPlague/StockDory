//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PERFTRUNNER_H
#define STOCKDORY_PERFTRUNNER_H

#include <chrono>
#include <iostream>

#include <nanothread/nanothread.h>

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

        public:
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

                if (Sync || depth < 5) {
                    nodes += PLayer::template PerftLoop<Pawn  >(board, depth, pin, check, pawns  );
                    nodes += PLayer::template PerftLoop<Knight>(board, depth, pin, check, knights);
                    nodes += PLayer::template PerftLoop<Bishop>(board, depth, pin, check, bishops);
                    nodes += PLayer::template PerftLoop<Rook  >(board, depth, pin, check, rooks  );
                    nodes += PLayer::template PerftLoop<Queen >(board, depth, pin, check, queens );
                    nodes += PLayer::template PerftLoop<King  >(board, depth, pin, check, kings  );
                } else {
                    std::array<uint64_t             , 6> result     = {};
                    std::array<std::function<void()>, 6> perftLoops = {
                        [pawns  , depth, &board, &pin, &check, &result] -> void
                        {
                            Board b = board;
                            result[Pawn  ] = PLayer::template PerftLoop<Pawn  >(b, depth, pin, check, pawns  );
                        },
                        [knights, depth, &board, &pin, &check, &result] -> void
                        {
                            Board b = board;
                            result[Knight] = PLayer::template PerftLoop<Knight>(b, depth, pin, check, knights);
                        },
                        [bishops, depth, &board, &pin, &check, &result] -> void
                        {
                            Board b = board;
                            result[Bishop] = PLayer::template PerftLoop<Bishop>(b, depth, pin, check, bishops);
                        },
                        [rooks  , depth, &board, &pin, &check, &result] -> void
                        {
                            Board b = board;
                            result[Rook  ] = PLayer::template PerftLoop<Rook  >(b, depth, pin, check, rooks  );
                        },
                        [queens , depth, &board, &pin, &check, &result] -> void
                        {
                            Board b = board;
                            result[Queen ] = PLayer::template PerftLoop<Queen >(b, depth, pin, check, queens );
                        },
                        [kings  , depth, &board, &pin, &check, &result] -> void
                        {
                            Board b = board;
                            result[King  ] = PLayer::template PerftLoop<King  >(b, depth, pin, check, kings  );
                        }
                    };

                    using Block = drjit::blocked_range<uint8_t>;
                    drjit::parallel_for(
                        Block (0, 6, 1),
                        [&perftLoops](const Block block) -> void
                        {
                            perftLoops[block.begin()]();
                        }
                    );

                    for (size_t i = 0; i < 6; i++) nodes += result[i];
                }
            }

            // if (TT) {
            //     const ZobristHash hash = board.Zobrist();
            //     PEntry& entry = TranspositionTable[hash];
            //     entry.Insert(hash, depth, nodes);
            // }

            return nodes;
        }

        private:
        template<Piece Piece, Color Color, bool Divide, bool Sync = false, bool TT = false>
        static inline uint64_t PerftLoop(Board&             board, const uint8_t      depth,
                                         const PinBitBoard& pin, const CheckBitBoard& check,
                                         BitBoardIterator   pIterator)
        {
            uint64_t nodes = 0;

            using PLayer = PerftLayer<Opposite(Color), false, Sync, TT>;
            using BLayer = BoardLayer<TT ? PERFT | ZOBRIST : STANDARD>;

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
                std::array<Square  , 8> psq    = {};
                std::array<uint64_t, 8> result = {};

                const uint8_t count  = pIterator.ToArray(psq);

                using Block = drjit::blocked_range<uint8_t>;

                auto Loop = [depth, &board, &pin, &check, &psq](const Block block) -> uint64_t
                {
                    const uint8_t start = block.begin();
                    const uint8_t end   = block.  end();

                    const uint8_t nextDepth = depth - 1;

                    Board parallelBoard = board;

                    uint64_t parallelNodes = 0;

                    for (uint8_t i = start; i < end; i++) {
                        const Square sq = psq[i];

                        MoveList<Piece, Color> moves (parallelBoard, sq, pin, check);
                        if (moves.Count() < 1) continue;

                        BitBoardIterator mIterator = moves.Iterator();

                        for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                            if (moves.Promotion(sq)) {
                                PreviousState  state       = BLayer::Move (parallelBoard, sq, m, Queen );
                                const uint64_t queenNodes  = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += queenNodes;

                                if (Divide) LogMove<Queen >(sq, m, queenNodes);

                                state                      = BLayer::Move (parallelBoard, sq, m, Rook  );
                                const uint64_t rookNodes   = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, sq, m);
                                parallelNodes += rookNodes;

                                if (Divide) LogMove<Rook  >(sq, m, rookNodes);

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

                drjit::parallel_for(
                    Block(0, count, 1),
                    [&Loop, &result](const Block block) -> void
                    {
                        result[block.begin()] = Loop(block);
                    }
                );

                for (size_t i = 0; i < 8; i++) nodes += result[i];
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
            static const std::regex comma ("(\\d)(?=(\\d{3})+(?!\\d))");

            std::cout << "Running PERFT @ depth " << static_cast<uint32_t>(depth) << " ";
            std::cout << "[Maximum Concurrency: " << ThreadPool.get_thread_count() << "t]:";
            std::cout << std::endl;

            const auto     start = std::chrono::high_resolution_clock::now();
            const uint64_t nodes =
                PerftBoard.ColorToMove() == White
                ? Perft<White, Divide, false, TT>(PerftBoard, depth)
                : Perft<Black, Divide, false, TT>(PerftBoard, depth);
            const auto     stop  = std::chrono::high_resolution_clock::now();
            const auto     time  = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

            const double_t t   = static_cast<double_t>(time) / 1000000;
            const uint64_t nps = static_cast<uint64_t>(nodes / t     );

            std::cout << std::endl;

            std::cout << "Nodes searched: " << std::regex_replace(std::to_string(nodes), comma, "$1,");
            std::cout << std::endl;
            std::cout << "Time taken: " << t << "s";
            std::cout << std::endl;
            std::cout << "Speed: " << std::regex_replace(std::to_string(nps), comma, "$1,") << " nps";
            std::cout << std::endl;
        }

    };

} // Perft

StockDory::Board StockDory::PerftRunner::PerftBoard = Board();
// StockDory::TranspositionTable<PEntry> StockDory::Perft::PerftRunner::TranspositionTable =
// StockDory::TranspositionTable<PEntry>(0);

#endif //STOCKDORY_PERFTRUNNER_H
