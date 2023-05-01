#include <iostream>

#include "../Backend/Move/TableSetup.h"

#include "../Backend/Move/MoveList.h"

#include "UI/ScreenManager.h"

#include "Perft/PerftRunner.h"

int main(int argc, char* argv[])
{
    TableSetup();

    if (argc < 3) {
        std::cerr << "Usage: ./StockDory [FEN string] [PerftDepth uint] [TTSize uint|null]" << std::endl;
        return 1;  // Return with error code
    }

    std::string fen = argv[1];
    uint8_t depth;
    try {
        depth = static_cast<uint8_t>(std::stoi(argv[2]));
    } catch (const std::exception& e) {
        std::cerr << "Invalid Perft depth. Must be an integer." << std::endl;
        return 1;  // Return with error code
    }

    uint64_t size = 0;
    if (argc == 4) {
        try {
            size = static_cast<uint64_t>(std::stoull(argv[3]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid TT size. Must be an integer." << std::endl;
            return 1;  // Return with error code
        }
    }

    auto board = StockDory::Board(fen);

    StockDory::ScreenManager::DrawBoard(board);
    StockDory::ScreenManager::Refresh();

    StockDory::Perft::PerftRunner::SetBoard(board);

    if (size != 0) {
        StockDory::Perft::PerftRunner::SetTranspositionTable(size);
        StockDory::Perft::PerftRunner::Perft<false, true>(depth);
    } else {
        StockDory::Perft::PerftRunner::Perft<false>(depth);
    }

    return 0;
}
