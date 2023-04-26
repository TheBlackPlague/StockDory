//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PERFTRUNNER_H
#define STOCKDORY_PERFTRUNNER_H

#include <iostream>
#include <chrono>

#include "../../Backend/Board.h"
#include "../../Backend/Util.h"
#include "../../Backend/Move/MoveList.h"

namespace StockDory::Perft
{

    class PerftRunner
    {

        private:
            static Board PerftBoard;

            template<::Color Color, bool Divide>
            static inline uint64_t Perft(const uint8_t depth)
            {
                uint64_t nodes = 0;

                const PinBitBoard   pin   = PerftBoard.Pin  <Color, Opposite(Color)>();
                const CheckBitBoard check = PerftBoard.Check<       Opposite(Color)>();

                if (check.DoubleCheck) {
                    const BitBoardIterator kings (PerftBoard.PieceBoard<Color>(Piece::King));
                    nodes += PerftPieceExpansion<King, Color, Divide>(depth, pin, check, kings);
                    return nodes;
                }

                const BitBoardIterator pawns   (PerftBoard.PieceBoard<Color>(Piece::Pawn  ));
                const BitBoardIterator knights (PerftBoard.PieceBoard<Color>(Piece::Knight));
                const BitBoardIterator bishops (PerftBoard.PieceBoard<Color>(Piece::Bishop));
                const BitBoardIterator rooks   (PerftBoard.PieceBoard<Color>(Piece::Rook  ));
                const BitBoardIterator queens  (PerftBoard.PieceBoard<Color>(Piece::Queen ));
                const BitBoardIterator kings   (PerftBoard.PieceBoard<Color>(Piece::King  ));

                nodes += PerftPieceExpansion<Pawn  , Color, Divide>(depth, pin, check, pawns  );
                nodes += PerftPieceExpansion<Knight, Color, Divide>(depth, pin, check, knights);
                nodes += PerftPieceExpansion<Bishop, Color, Divide>(depth, pin, check, bishops);
                nodes += PerftPieceExpansion<Rook  , Color, Divide>(depth, pin, check, rooks  );
                nodes += PerftPieceExpansion<Queen , Color, Divide>(depth, pin, check, queens );
                nodes += PerftPieceExpansion<King  , Color, Divide>(depth, pin, check, kings  );

                return nodes;
            }

            template<Piece Piece, ::Color Color, bool Divide>
            static inline uint64_t PerftPieceExpansion(const uint8_t depth,
                                                       const PinBitBoard& pin, const CheckBitBoard& check,
                                                       BitBoardIterator pIterator)
            {
                uint64_t nodes = 0;

                if (depth == 1) for (Square sq = pIterator.Value(); sq != Square::NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (PerftBoard, sq, pin, check);
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
                }

                for (Square sq = pIterator.Value(); sq != Square::NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (PerftBoard, sq, pin, check);

                    BitBoardIterator mIterator = moves.Iterator();

                    for (Square m = mIterator.Value(); m != Square::NASQ; m = mIterator.Value()) {
                        if (moves.Promotion(sq)) {
                            PreviousState state = PerftBoard.Move<MoveType::NAMT>(sq, m, Queen);
                            const uint64_t queenNodes = Perft<Opposite(Color), false>(depth - 1);
                            PerftBoard.UndoMove<MoveType::NAMT>(state, sq, m);
                            nodes += queenNodes;

                            if (Divide) LogMove<Queen >(sq, m, queenNodes);

                            state = PerftBoard.Move<MoveType::NAMT>(sq, m, Rook);
                            const uint64_t rookNodes = Perft<Opposite(Color), false>(depth - 1);
                            PerftBoard.UndoMove<MoveType::NAMT>(state, sq, m);
                            nodes += rookNodes;

                            if (Divide) LogMove<Rook  >(sq, m, rookNodes);

                            state = PerftBoard.Move<MoveType::NAMT>(sq, m, Bishop);
                            const uint64_t bishopNodes = Perft<Opposite(Color), false>(depth - 1);
                            PerftBoard.UndoMove<MoveType::NAMT>(state, sq, m);
                            nodes += bishopNodes;

                            if (Divide) LogMove<Bishop>(sq, m, bishopNodes);

                            state = PerftBoard.Move<MoveType::NAMT>(sq, m, Knight);
                            const uint64_t knightNodes = Perft<Opposite(Color), false>(depth - 1);
                            PerftBoard.UndoMove<MoveType::NAMT>(state, sq, m);
                            nodes += knightNodes;

                            if (Divide) LogMove<Knight>(sq, m, knightNodes);
                        } else {
                            const PreviousState state = PerftBoard.Move<MoveType::NAMT>(sq, m);
                            const uint64_t perftNodes = Perft<Opposite(Color), false>(depth - 1);
                            PerftBoard.UndoMove<MoveType::NAMT>(state, sq, m);
                            nodes += perftNodes;

                            if (Divide) LogMove(sq, m, perftNodes);
                        }
                    }
                }

                return nodes;
            }

            template<Piece Promotion = NAP>
            static void LogMove(const Square from, const Square to, const uint64_t nodes)
            {
                std::cout << Util::SquareToString(from) << Util::SquareToString(to);
                if (Promotion != NAP) std::cout << static_cast<char>(tolower(FirstLetter(Promotion)));
                std::cout << ": " << nodes << std::endl;
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

            template <bool Divide>
            static void Perft(const uint8_t depth)
            {
                std::cout << "Running PERFT @ depth " << static_cast<uint32_t>(depth) << ":" << std::endl;

                auto start = std::chrono::high_resolution_clock::now();
                const uint64_t nodes =
                        PerftBoard.ColorToMove() == White ?
                        Perft<White, Divide>(depth) :
                        Perft<Black, Divide>(depth);
                auto stop  = std::chrono::high_resolution_clock::now();
                auto time = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();

                std::cout << "Searched " << nodes << " nodes. (" << time << "µs)" << std::endl;
            }

    };

} // Perft

StockDory::Board StockDory::Perft::PerftRunner::PerftBoard = StockDory::Board();

#endif //STOCKDORY_PERFTRUNNER_H
