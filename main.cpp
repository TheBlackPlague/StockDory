#include <iostream>
#include "Backend/Board.h"         // Include Board.h for chess board representation
#include "Backend/Type/Square.h"   // Include Square.h to use the Square enum
#include "SimplifiedMoveList.h"    // Include your SimplifiedMoveList class
#include "Backend/Type/Color.h"
#include "Engine.h"

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
    // for (uint8_t i = 0; i < moveList.Count(); i++) {
    //     Move move = moveList[i];  // Get the i-th move
    //
    //     // Get the starting and destination squares of the move
    //     Square from = move.From();
    //     Square to = move.To();
    //
    //     // Print the move in some format (you can customize how the move is printed)
    //     std::cout << "Move " << static_cast<int>(i + 1) << ": "
    //           << "from " << squareToString(from) << " to " << squareToString(to) << std::endl;
    //     PreviousState prevState = chessBoard.Move<0>(from, to);
    //
    //     // Print the FEN string after making the move
    //     std::string currentFEN = chessBoard.Fen();
    //     std::cout << "Board state after move: " << currentFEN << std::endl;
    //
    //     // Undo the move by passing the stored state and move details
    //     chessBoard.UndoMove<0>(prevState, from, to);
    //
    //     // Print the FEN string after undoing the move
    //     std::string revertedFEN = chessBoard.Fen();
    //     std::cout << "Board state after undoing move: " << revertedFEN << std::endl;
    // }
    //
    //
    // // Optional: You can print the current board state in FEN after performing a move
    // std::string currentFEN = chessBoard.Fen();
    // std::cout << "Board state: " << currentFEN << std::endl;

    Engine engine;
    std::pair<Move, float> result = engine.minimax(chessBoard, 3);
    std::cout << "Result is: " << squareToString(result.first.From()) << " to " << squareToString(result.first.To()) << " with score " << result.second;


    return 0;
}
