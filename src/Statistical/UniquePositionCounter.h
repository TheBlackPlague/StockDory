//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UNIQUEPOSITIONCOUNTER_H
#define STOCKDORY_UNIQUEPOSITIONCOUNTER_H

#include <iostream>
#include <chrono>
#include <cstdint>
#include <array>

#include "../Backend/Board.h"
#include "../Backend/Move/MoveList.h"

#include "../External/hash_set8.hpp"

namespace StockDory
{

    struct CompressedPosition
    {

        private:
            std::array<std::array<BitBoard, 6>, 2> BB {};

            Square EnPassantTarget = NASQ;

            uint8_t CastlingRightAndColorToMove = 0;

            ZobristHash Hash = 0ULL;

        public:
            explicit CompressedPosition(const Board& board)
            {
                for (Piece p = Pawn; p != NAP; p = Next(p)) {
                    BB[White][p] = board.PieceBoard<White>(p);
                    BB[Black][p] = board.PieceBoard<Black>(p);
                }

                EnPassantTarget = board.EnPassantSquare();

                CastlingRightAndColorToMove = board.ColorToMove() << 4;

                CastlingRightAndColorToMove |= board.CastlingRightK<White>() ? 0x8 : 0x0;
                CastlingRightAndColorToMove |= board.CastlingRightQ<White>() ? 0x4 : 0x0;
                CastlingRightAndColorToMove |= board.CastlingRightK<Black>() ? 0x2 : 0x0;
                CastlingRightAndColorToMove |= board.CastlingRightQ<Black>() ? 0x1 : 0x0;

                Hash = board.Zobrist();
            }

            [[nodiscard]]
            inline ZobristHash GetHash() const
            {
                return Hash;
            }

            inline bool operator==(const CompressedPosition& o) const
            {
                return Hash == o.Hash && BB == o.BB && EnPassantTarget == o.EnPassantTarget &&
                       CastlingRightAndColorToMove == o.CastlingRightAndColorToMove;
            }

    };

    using HashSet = emhash8::HashSet<CompressedPosition,
        decltype([](const CompressedPosition& p) {
            return static_cast<size_t>(p.GetHash());
        }),
        decltype([](const CompressedPosition& l, const CompressedPosition& r) {
            return l == r;
        })>;

    class UpcRunner
    {

        private:
            template<Color Color, bool Zobrist>
            struct UpcLayer
            {

                public:
                    static inline void Collect(Board& board, const uint8_t depth)
                    {
                        UpcRunner::Collect<Color, Zobrist>(board, depth);
                    }

                    template<Piece Piece>
                    static inline void CollectionLoop(Board& board, const uint8_t depth,
                                                      const PinBitBoard& pin, const CheckBitBoard& check,
                                                      const BitBoardIterator& iterator)
                    {
                        UpcRunner::CollectionLoop<Piece, Color, Zobrist>(board, depth, pin, check, iterator);
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

            static Board UpcBoard;

            static HashSet UniquePositionSet;

            template<Color Color, bool Zobrist>
            static inline void Collect(Board& board, const uint8_t depth)
            {
                using ULayer = UpcLayer<Color, Zobrist>;

                if (depth == 0) {
                    const CompressedPosition position (board);
                    UniquePositionSet.insert(position);
                    return;
                }

                const PinBitBoard   pin   = board.Pin  <Color, Opposite(Color)>();
                const CheckBitBoard check = board.Check<       Opposite(Color)>();

                if (check.DoubleCheck) {
                    const BitBoardIterator kings (board.PieceBoard<Color>(King));
                    ULayer::template CollectionLoop<King>(board, depth, pin, check, kings);
                } else {
                    const BitBoardIterator pawns   (board.PieceBoard<Color>(Pawn  ));
                    const BitBoardIterator knights (board.PieceBoard<Color>(Knight));
                    const BitBoardIterator bishops (board.PieceBoard<Color>(Bishop));
                    const BitBoardIterator rooks   (board.PieceBoard<Color>(Rook  ));
                    const BitBoardIterator queens  (board.PieceBoard<Color>(Queen ));
                    const BitBoardIterator kings   (board.PieceBoard<Color>(King  ));

                    ULayer::template CollectionLoop<Pawn  >(board, depth, pin, check, pawns  );
                    ULayer::template CollectionLoop<Knight>(board, depth, pin, check, knights);
                    ULayer::template CollectionLoop<Bishop>(board, depth, pin, check, bishops);
                    ULayer::template CollectionLoop<Rook  >(board, depth, pin, check, rooks  );
                    ULayer::template CollectionLoop<Queen >(board, depth, pin, check, queens );
                    ULayer::template CollectionLoop<King  >(board, depth, pin, check, kings  );
                }
            }

            template<Piece Piece, Color Color, bool Zobrist>
            static inline void CollectionLoop(Board& board, const uint8_t depth,
                                              const PinBitBoard& pin, const CheckBitBoard& check,
                                              BitBoardIterator pIterator)
            {
                using ULayer = UpcLayer<Opposite(Color), Zobrist>;
                using BLayer = BoardLayer<Zobrist ? (PERFT | ZOBRIST) : STANDARD>;

                for (Square sq = pIterator.Value(); sq != NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (board, sq, pin, check);

                    BitBoardIterator mIterator = moves.Iterator();

                    for (Square m = mIterator.Value(); m != NASQ; m = mIterator.Value()) {
                        if (moves.Promotion(sq)) {
                            for (::Piece promotion = Knight; promotion != King; promotion = Next(promotion)) {
                                const PreviousState state = BLayer::Move(board, sq, m, promotion);
                                ULayer::Collect(board, depth - 1);
                                BLayer::UndoMove(board, state, sq, m);
                            }
                        } else {
                            const PreviousState state = BLayer::Move (board, sq, m);
                            ULayer::Collect(board, depth - 1);
                            BLayer::UndoMove(board, state, sq, m);
                        }
                    }
                }
            }

        public:
            static void SetBoard(const std::string& fen)
            {
                UpcBoard = Board(fen);
            }

            template<bool Zobrist>
            static void UniquePositionCount(const uint8_t depth)
            {
                std::cout << "Counting number of unique positions at #";
                std::cout << static_cast<uint32_t>(depth) << " half-move." << std::endl;

                auto start = std::chrono::high_resolution_clock::now();
                if (UpcBoard.ColorToMove() == White) Collect<White, Zobrist>(UpcBoard, depth);
                else                                 Collect<Black, Zobrist>(UpcBoard, depth);
                auto stop = std::chrono::high_resolution_clock::now();
                auto time = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

                std::cout << "Counted " << UniquePositionSet.size();
                std::cout << " total unique positions. (" << time << "µs - Load Factor: ";
                std::cout << UniquePositionSet.load_factor() << "/" << UniquePositionSet.max_load_factor() << ")";
                std::cout << std::endl;
            }

    };

    StockDory::Board StockDory::UpcRunner::UpcBoard = StockDory::Board();
    HashSet StockDory::UpcRunner::UniquePositionSet = HashSet();

}

#endif //STOCKDORY_UNIQUEPOSITIONCOUNTER_H
