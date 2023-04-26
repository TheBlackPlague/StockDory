#include <iostream>

#include "../Backend/Move/TableSetup.h"

#include "../Backend/Move/MoveList.h"

#include "UI/ScreenManager.h"

#include "Perft/PerftRunner.h"

int main()
{
    TableSetup();

    const std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    auto board = StockDory::Board(fen);

    StockDory::ScreenManager::DrawBoard(board);
    StockDory::ScreenManager::Refresh();

    StockDory::Perft::PerftRunner::SetBoard(board);

    StockDory::Perft::PerftRunner::Perft<false>(7);

    return 0;
}
