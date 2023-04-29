#include <iostream>

#include "../Backend/Move/TableSetup.h"

#include "../Backend/Move/MoveList.h"

#include "UI/ScreenManager.h"

#include "Perft/PerftRunner.h"

int main(int argc, char* argv[])
{
    TableSetup();

//    const std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    if (argc != 3) {
        std::cerr << "Usage: ./StockDory [FEN string] [Perft depth]" << std::endl;
        return 1;  // Return with error code
    }

    std::string fen = argv[1];
    int depth;
    try {
        depth = std::stoi(argv[2]);
    } catch (const std::exception& e) {
        std::cerr << "Invalid Perft depth. Must be an integer." << std::endl;
        return 1;  // Return with error code
    }

    auto board = StockDory::Board(fen);

    StockDory::ScreenManager::DrawBoard(board);
    StockDory::ScreenManager::Refresh();

    StockDory::Perft::PerftRunner::SetBoard(board);

    StockDory::Perft::PerftRunner::Perft<false>(depth);

    return 0;
}
