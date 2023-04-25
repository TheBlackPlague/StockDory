#include <iostream>

#include "../Backend/Move/TableSetup.h"

#include "../Backend/Move/MoveList.h"

#include "UI/ScreenManager.h"

#include "Perft/PerftRunner.h"

int main()
{
    TableSetup();

    const std::string fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

    auto board = StockDory::Board(fen);

    StockDory::ScreenManager::DrawBoard(board);

    StockDory::ScreenManager::Refresh();

    std::cout << std::endl;

    StockDory::Perft::PerftRunner::SetBoard(fen);

    StockDory::Perft::PerftRunner::Perft<true>(1);

    return 0;
}
