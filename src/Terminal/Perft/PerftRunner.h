//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_PERFTRUNNER_H
#define STOCKDORY_PERFTRUNNER_H

#include <chrono>
#include <functional>
#include <iostream>
#include <math.h>
#include <regex>

#include <nanothread/nanothread.h>

#include "../../Backend/Board.h"
#include "../../Backend/Misc.h"
#include "../../Backend/ThreadPool.h"
#include "../../Backend/Move/MoveList.h"

namespace StockDory
{

    class PerftRunner
    {

        static PerftBoard InternalBoard;

        template<Color Color, bool Divide, bool Sync = false>
        struct PerftLayer
        {

            static inline uint64_t Perft(PerftBoard& board, const uint8_t depth)
            {
                return PerftRunner::Perft<Color, Divide, Sync>(board, depth);
            }

            template<Piece Piece>
            static inline uint64_t PerftLoop(       PerftBoard&      board, const uint8_t        depth,
                                             const PinBitBoard&      pin,   const CheckBitBoard& check,
                                             const BitBoardIterator& iterator)
            {
                return PerftRunner::PerftLoop<Piece, Color, Divide, Sync>(board, depth, pin, check, iterator);
            }

        };

        template<MoveType T>
        struct BoardLayer
        {

            static inline PreviousState Move(PerftBoard& board, const ::Move move)
            {
                return board.Move<T>(move);
            }

            static inline void UndoMove(PerftBoard& board, const PreviousState& state, const ::Move move)
            {
                board.UndoMove<T>(state, move);
            }

        };

        public:
        template<Color Color, bool Divide, bool Sync = false>
        static inline uint64_t Perft(PerftBoard& board, const uint8_t depth)
        {
            uint64_t nodes = 0;
            using PLayer   = PerftLayer<Color, Divide, Sync>;

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
                    Array<uint64_t             , 6> result     = {};
                    Array<std::function<void()>, 6> perftLoops = {
                        [pawns  , depth, &board, &pin, &check, &result] -> void
                        {
                            PerftBoard b = board;
                            result[Pawn  ] = PLayer::template PerftLoop<Pawn  >(b, depth, pin, check, pawns  );
                        },
                        [knights, depth, &board, &pin, &check, &result] -> void
                        {
                            PerftBoard b = board;
                            result[Knight] = PLayer::template PerftLoop<Knight>(b, depth, pin, check, knights);
                        },
                        [bishops, depth, &board, &pin, &check, &result] -> void
                        {
                            PerftBoard b = board;
                            result[Bishop] = PLayer::template PerftLoop<Bishop>(b, depth, pin, check, bishops);
                        },
                        [rooks  , depth, &board, &pin, &check, &result] -> void
                        {
                            PerftBoard b = board;
                            result[Rook  ] = PLayer::template PerftLoop<Rook  >(b, depth, pin, check, rooks  );
                        },
                        [queens , depth, &board, &pin, &check, &result] -> void
                        {
                            PerftBoard b = board;
                            result[Queen ] = PLayer::template PerftLoop<Queen >(b, depth, pin, check, queens );
                        },
                        [kings  , depth, &board, &pin, &check, &result] -> void
                        {
                            PerftBoard b = board;
                            result[King  ] = PLayer::template PerftLoop<King  >(b, depth, pin, check, kings  );
                        }
                    };

                    ThreadPool.For(
                        Block(0, 6),
                        [&perftLoops](const Block block) -> void
                        {
                            perftLoops[block.begin()]();
                        }
                    );

                    for (size_t i = 0; i < 6; i++) nodes += result[i];
                }
            }

            return nodes;
        }

        private:
        template<Piece Piece, Color Color, bool Divide, bool Sync = false>
        static inline uint64_t PerftLoop(PerftBoard&         board, const uint8_t      depth,
                                         const PinBitBoard& pin, const CheckBitBoard& check,
                                         BitBoardIterator   pIterator)
        {
            uint64_t nodes = 0;

            using PLayer = PerftLayer<Opposite(Color), false, Sync>;
            using BLayer = BoardLayer<STANDARD>;

            if (depth == 1)
                for (Square sq = pIterator.Value(); sq != NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color, BoardType::Perft> moves (board, sq, pin, check);
                    const uint8_t count = moves.Count();

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
                    const MoveList<Piece, Color, BoardType::Perft> moves (board, sq, pin, check);

                    BitBoardIterator mIterator = moves.Iterator();

                    for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                        if (moves.Promotion(sq)) {
                            ::Move move                = board.CreateMove<Pawn>(sq, m, Queen );
                            PreviousState  state       = BLayer::Move(board, move);
                            const uint64_t queenNodes  = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, move);
                            nodes += queenNodes;

                            if (Divide) LogMove<Queen >(sq, m,  queenNodes);

                            move                       = board.CreateMove<Pawn>(sq, m, Rook  );
                            state                      = BLayer::Move(board, move);
                            const uint64_t rookNodes   = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, move);
                            nodes += rookNodes;

                            if (Divide) LogMove<Rook  >(sq, m,   rookNodes);

                            move                       = board.CreateMove<Pawn>(sq, m, Bishop);
                            state                      = BLayer::Move(board, move);
                            const uint64_t bishopNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, move);
                            nodes += bishopNodes;

                            if (Divide) LogMove<Bishop>(sq, m, bishopNodes);

                            move                       = board.CreateMove<Pawn>(sq, m, Knight);
                            state                      = BLayer::Move(board, move);
                            const uint64_t knightNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, move);
                            nodes += knightNodes;

                            if (Divide) LogMove<Knight>(sq, m, knightNodes);
                        } else {
                            const ::Move        move       = board.CreateMove<Piece>(sq, m);
                            const PreviousState state      = BLayer::Move(board, move);
                            const uint64_t      perftNodes = PLayer::Perft(board, depth - 1);
                            BLayer::UndoMove(board, state, move);
                            nodes += perftNodes;

                            if (Divide) LogMove(sq, m, perftNodes);
                        }
                    }
                }
            else {
                Array<Square  , 10> psq    = {};
                Array<uint64_t, 10> result = {};

                const uint8_t count  = pIterator.ToArray(psq);

                auto Loop = [depth, &board, &pin, &check, &psq](const Block block) -> uint64_t
                {
                    const uint8_t start = block.begin();
                    const uint8_t end   = block.  end();

                    const uint8_t nextDepth = depth - 1;

                    PerftBoard parallelBoard = board;

                    uint64_t parallelNodes = 0;

                    for (uint8_t i = start; i < end; i++) {
                        const Square sq = psq[i];

                        MoveList<Piece, Color, BoardType::Perft> moves (parallelBoard, sq, pin, check);
                        if (moves.Count() < 1) continue;

                        BitBoardIterator mIterator = moves.Iterator();

                        for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                            if (moves.Promotion(sq)) {
                                const ::Move   queenMove   = parallelBoard.CreateMove<Piece>(sq, m, Queen );
                                PreviousState  state       = BLayer::Move(parallelBoard, queenMove);
                                const uint64_t queenNodes  = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, queenMove);
                                parallelNodes += queenNodes;

                                if (Divide) LogMove<Queen >(sq, m, queenNodes);

                                const ::Move   rookMove    = parallelBoard.CreateMove<Piece>(sq, m, Rook  );
                                state                      = BLayer::Move(parallelBoard, rookMove);
                                const uint64_t rookNodes   = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, rookMove);
                                parallelNodes += rookNodes;

                                if (Divide) LogMove<Rook  >(sq, m, rookNodes);

                                const ::Move   bishopMove  = parallelBoard.CreateMove<Piece>(sq, m, Bishop);
                                state                      = BLayer::Move(parallelBoard, bishopMove);
                                const uint64_t bishopNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, bishopMove);
                                parallelNodes += bishopNodes;

                                if (Divide) LogMove<Bishop>(sq, m, bishopNodes);

                                const ::Move   knightMove  = parallelBoard.CreateMove<Piece>(sq, m, Knight);
                                state                      = BLayer::Move(parallelBoard, knightMove);
                                const uint64_t knightNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, knightMove);
                                parallelNodes += knightNodes;

                                if (Divide) LogMove<Knight>(sq, m, knightNodes);
                            } else {
                                const ::Move        move       = parallelBoard.CreateMove<Piece>(sq, m);
                                const PreviousState state      = BLayer::Move(parallelBoard, move);
                                const uint64_t      perftNodes = PLayer::Perft(parallelBoard, nextDepth);
                                BLayer::UndoMove(parallelBoard, state, move);
                                parallelNodes += perftNodes;

                                if (Divide) LogMove(sq, m, perftNodes);
                            }
                        }
                    }

                    return parallelNodes;
                };

                ThreadPool.For(
                    Block(0, count),
                    [&Loop, &result](const Block block) -> void
                    {
                        result[block.begin()] = Loop(block);
                    }
                );

                for (size_t i = 0; i < count; i++) nodes += result[i];
            }

            return nodes;
        }

        template<Piece Promotion = NAP>
        static void LogMove(const Square from, const Square to, const uint64_t nodes)
        {
            std::string logEntry = ToString(from) + ToString(to);
            if (Promotion != NAP) logEntry += static_cast<char>(tolower(FirstLetter(Promotion)));
            logEntry += ": " + std::to_string(nodes) + "\n";
            std::cout << logEntry;
        }

        public:
        static void SetBoard(const std::string& fen)
        {
            InternalBoard = PerftBoard(fen);
        }

        static void SetBoard(const Board& board)
        {
            InternalBoard = PerftBoard(board);
        }

        template<bool Divide>
        static void Perft(const uint8_t depth)
        {
            static const std::regex comma ("(\\d)(?=(\\d{3})+(?!\\d))");

            std::cout << "Running PERFT @ depth " << static_cast<uint32_t>(depth) << " ";
            std::cout << "[Maximum Concurrency: " << ThreadPool.Size() << "t]:";
            std::cout << std::endl;

            const auto     start = std::chrono::high_resolution_clock::now();
                  uint64_t nodes = 0;

            if (depth != 0) {
                if (ThreadPool.Size() > 1)
                    nodes = InternalBoard.ColorToMove() == White
                        ? Perft<White, Divide, false>(InternalBoard, depth)
                        : Perft<Black, Divide, false>(InternalBoard, depth);
                else
                    nodes = InternalBoard.ColorToMove() == White
                        ? Perft<White, Divide, true >(InternalBoard, depth)
                        : Perft<Black, Divide, true >(InternalBoard, depth);
            } else  nodes = 1;

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

StockDory::PerftBoard StockDory::PerftRunner::InternalBoard = PerftBoard();

#endif //STOCKDORY_PERFTRUNNER_H
