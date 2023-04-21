#include <iostream>

#include "../Backend/Move/TableSetup.h"

#include "../Backend/Board.h"

#include "UI/ScreenManager.h"

int main()
{
    TableSetup();

    StockDory::Board board = StockDory::Board();

    StockDory::ScreenManager::DrawBoard(board);

    StockDory::ScreenManager::Refresh();

    return 0;
}
