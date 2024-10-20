#include <iostream>
#include "Backend/Board.h"         // Include Board.h for chess board representation
#include "Backend/Type/Square.h"   // Include Square.h to use the Square enum
#include "SimplifiedMoveList.h"    // Include your SimplifiedMoveList class
#include "Backend/Type/Color.h"

std::string squareToString(Square square) {
    return std::string(1, File(square)) + std::string(1, Rank(square));
}

int main() {
    // Initialize the chess board with the standard starting position
    StockDory::Board chessBoard;

    // Create a SimplifiedMoveList for the current board position (for White)
    StockDory::SimplifiedMoveList<Color::White> moveList(chessBoard);  // Generating moves for White

    // Print the number of available moves
    std::cout << "Number of possible moves: " << static_cast<int>(moveList.Count()) << std::endl;

    // Iterate through all moves and print them
    for (uint8_t i = 0; i < moveList.Count(); i++) {
        Move move = moveList[i];  // Get the i-th move

        // Get the starting and destination squares of the move
        Square from = move.From();
        Square to = move.To();

        // Print the move in some format (you can customize how the move is printed)
        std::cout << "Move " << static_cast<int>(i + 1) << ": "
              << "from " << squareToString(from) << " to " << squareToString(to) << std::endl;
    }

    // Optional: You can print the current board state in FEN after performing a move
    std::string currentFEN = chessBoard.Fen();
    std::cout << "Board state: " << currentFEN << std::endl;

    return 0;
}
