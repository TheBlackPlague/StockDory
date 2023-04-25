//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_PERFTRUNNER_H
#define STOCKDORY_PERFTRUNNER_H

#include <iostream>

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
                            if (moves.Promotion(sq)) LogMove<true >(sq, m, 1);
                            else                     LogMove<false>(sq, m, 1);
                        }
                    }
                }

                for (Square sq = pIterator.Value(); sq != Square::NASQ; sq = pIterator.Value()) {
                    const MoveList<Piece, Color> moves (PerftBoard, sq, pin, check);

                    BitBoardIterator mIterator = moves.Iterator();

                    for (Square m = mIterator.Value(); m != Square::NASQ; m = mIterator.Value()) {
                        if (moves.Promotion(sq)) {
                            for (enum Piece p = Piece::Knight; p < Piece::King; p = Next(p)) {
                                // Make promotional move.
                                nodes += Perft<Opposite(Color), false>(depth - 1);
                                // Unmake promotional move.
                            }
                        } else {
                            // Make Move.
                            nodes += Perft<Opposite(Color), false>(depth - 1);
                            // Unmake Move.
                        }
                    }
                }

                return nodes;
            }

            template<bool Promotion>
            static void LogMove(const Square from, const Square to, const uint64_t nodes)
            {
                std::cout << Util::SquareToString(from) << Util::SquareToString(to) << ": ";
                std::cout << ((Promotion ? 4 : 1) * nodes) << std::endl;
            }

        public:
            static void SetBoard(const std::string& fen)
            {
                PerftBoard = Board(fen);
            }

            template <bool Divide>
            static void Perft(const uint8_t depth)
            {
                std::cout << "Running PERFT @ depth " << static_cast<uint32_t>(depth) << ":" << std::endl;

                const uint64_t nodes =
                        PerftBoard.ColorToMove() == White ?
                        Perft<White, Divide>(depth) :
                        Perft<Black, Divide>(depth);

                std::cout << "Searched " << nodes << " nodes." << std::endl;
            }

    };

} // Perft

StockDory::Board StockDory::Perft::PerftRunner::PerftBoard = StockDory::Board();

#endif //STOCKDORY_PERFTRUNNER_H
