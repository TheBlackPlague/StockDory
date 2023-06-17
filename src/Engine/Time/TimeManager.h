//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_TIMEMANAGER_H
#define STOCKDORY_TIMEMANAGER_H

#include "../Move/OrderedMoveList.h"
#include "TimeControl.h"

#include "../../Backend/Board.h"

namespace StockDory
{

    struct TimeData
    {

        uint64_t WhiteTime     ;
        uint64_t BlackTime     ;
        uint64_t WhiteIncrement;
        uint64_t BlackIncrement;
        uint16_t MovesToGo     ;

    };

    class TimeManager
    {

        private:
            constexpr static uint8_t TimePartition = 20;
            constexpr static uint8_t TimeOverhead  = 10;

            constexpr static uint8_t IncrementPartitionNumerator   = 3;
            constexpr static uint8_t IncrementPartitionDenominator = 4;

            constexpr static uint8_t  TimeDifferenceNumerator   =    1;
            constexpr static uint8_t  TimeDifferenceDenominator =    2;
            constexpr static uint64_t TimeDifferenceThreshold   = 2000;

            constexpr static uint8_t PieceCountBase        = 32;
            constexpr static uint8_t PieceCountNumerator   =  3;
            constexpr static uint8_t PieceCountDenominator =  5;

            constexpr static uint8_t  MoveCountBase        =  25;
            constexpr static uint8_t  MoveCountNumerator   =   3;
            constexpr static uint8_t  MoveCountDenominator =   4;
            constexpr static uint64_t MoveInstantTime      = 500;

            constexpr static uint8_t CheckNumerator   = 10;
            constexpr static uint8_t CheckDenominator =  7;

            constexpr static KillerTable  DummyKTable;
            constexpr static HistoryTable DummyHTable;

        public:
            static TimeControl Default()
            {
                return {};
            }

            static TimeControl Fixed(const uint64_t time)
            {
                return TimeControl(time, time);
            }

            static TimeControl Optimal(const Board& board, const TimeData& data)
            {
                const Color color = board.ColorToMove();

                const uint64_t time = color == White ? data.WhiteTime      : data.BlackTime     ;
                const uint64_t inc  = color == White ? data.WhiteIncrement : data.BlackIncrement;

                uint64_t actual = data.MovesToGo > 0 ? std::max(time / TimePartition, time / data.MovesToGo)
                                                              : time / TimePartition;

                actual += inc * IncrementPartitionNumerator / IncrementPartitionDenominator;

                actual += time - (color == White ? data.BlackTime : data.WhiteTime) > TimeDifferenceThreshold ?
                          time * TimeDifferenceNumerator / TimeDifferenceDenominator : 0;

                actual -= TimeOverhead;

                uint64_t optimal = actual;

                optimal = PieceCountAdjustment(board, optimal);
                optimal =  MoveCountAdjustment(board, optimal);
                optimal =      CheckAdjustment(board, optimal);

                std::cout << "Actual: " << actual << " Optimal: " << optimal << "\n";

                optimal = std::min(optimal, actual);

                return TimeControl(optimal, actual);
            }

        private:
            static uint64_t PieceCountAdjustment(const Board& board, const uint64_t optimal)
            {
                const uint8_t pieceCount =
                        Count(board.PieceBoard<White>(Pawn  ) | board.PieceBoard<Black>(Pawn  )) +
                        Count(board.PieceBoard<White>(Knight) | board.PieceBoard<Black>(Knight)) +
                        Count(board.PieceBoard<White>(Bishop) | board.PieceBoard<Black>(Bishop)) +
                        Count(board.PieceBoard<White>(Rook  ) | board.PieceBoard<Black>(Rook  )) +
                        Count(board.PieceBoard<White>(Queen ) | board.PieceBoard<Black>(Queen )) +
                        Count(board.PieceBoard<White>(King  ) | board.PieceBoard<Black>(King  ));

                return std::max(optimal * pieceCount          / PieceCountBase,
                                optimal * PieceCountNumerator / PieceCountDenominator);
            }

            static uint64_t MoveCountAdjustment(const Board& board, const uint64_t optimal)
            {
                uint8_t moveCount = board.ColorToMove() == White ?
                                    OrderedMoveList<White>(
                                            board, 0, DummyKTable, DummyHTable, NoMove
                                    ).Count() :
                                    OrderedMoveList<Black>(
                                            board, 0, DummyKTable, DummyHTable, NoMove
                                    ).Count() ;

                moveCount = std::min<uint8_t>(moveCount, MoveCountBase);

                return moveCount == 1 ? std::min<uint64_t>(MoveInstantTime, optimal) :
                                        std::max(optimal * moveCount          / MoveCountBase,
                                                 optimal * MoveCountNumerator / MoveCountDenominator);
            }

            static uint64_t CheckAdjustment(const Board& board, const uint64_t optimal)
            {
                const bool checked = board.ColorToMove() == White ? board.Checked<White>() : board.Checked<Black>();

                return checked ? optimal *   CheckNumerator / CheckDenominator :
                                 optimal * CheckDenominator /   CheckNumerator ;
            }

    };

} // StockDory

#endif //STOCKDORY_TIMEMANAGER_H
