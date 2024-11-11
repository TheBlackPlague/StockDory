// main.cpp
#include <iostream>
#include <limits>
#include <string>
#include <cstdlib> // For std::atoi
#include "Backend/Board.h"         // Include Board.h for chess board representation
#include "Backend/Type/Square.h"   // Include Square.h to use the Square enum
#include "SimplifiedMoveList.h"    // Include your SimplifiedMoveList class
#include "Backend/Type/Color.h"
#include "Engine.h"

// Function to convert a Square enum to its string representation (e.g., E2 -> "e2")
std::string squareToString(Square square) {
    return std::string(1, File(square)) + std::string(1, Rank(square));
}

int main(int argc, char* argv[]) {
    // Check if the depth argument is provided
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <depth>\n";
        std::cerr << "Please provide the search depth as a command-line argument.\n";
        return 1;
    }

    // Parse the depth from the first command-line argument
    int depth = std::atoi(argv[1]);

    // Validate the depth
    if (depth <= 0) {
        std::cerr << "Invalid depth: " << depth << ". Depth must be a positive integer.\n";
        return 1;
    }

    std::cout << "Starting Minimax with depth: " << depth << "\n";

    // Initialize the chess board with the standard starting position
    StockDory::Board chessBoard;

    Engine engine;

    // Determine which color is to move
    Color currentPlayer = chessBoard.ColorToMove();

    // Define a pair to hold the result (best move sequence and its score)
    std::pair<std::vector<Move>, float> result;

    // Execute the appropriate search algorithm based on the current player
    if (currentPlayer == White) {
        // Perform alpha-beta pruning for White
        result = engine.alphaBetaNega<White>(
            chessBoard,
            -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity(),
            depth
        );

        // Check if there is at least one move in the sequence
        if (!result.first.empty()) {
            Move bestMove = result.first.front();
            std::cout << "White's Best Move: " 
                      << squareToString(bestMove.From()) << " to " 
                      << squareToString(bestMove.To()) 
                      << " with score " << result.second << "\n";

            // Print the entire sequence of moves (best line)
            std::cout << "Best Line: ";
            for (const Move &move : result.first) {
                std::cout << squareToString(move.From()) << " to " 
                          << squareToString(move.To()) << ", ";
            }
            std::cout << "\n";
        } else {
            std::cout << "No moves available for White.\n";
        }
    }
    else if (currentPlayer == Black) {
        // Perform alpha-beta pruning for Black
        result = engine.alphaBetaNega<Black>(
            chessBoard,
            -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity(),
            depth
        );

        // Check if there is at least one move in the sequence
        if (!result.first.empty()) {
            Move bestMove = result.first.front();
            std::cout << "Black's Best Move: " 
                      << squareToString(bestMove.From()) << " to " 
                      << squareToString(bestMove.To()) 
                      << " with score " << result.second << "\n";

            // Print the entire sequence of moves (best line)
            std::cout << "Best Line: ";
            for (const Move &move : result.first) {
                std::cout << squareToString(move.From()) << " to " 
                          << squareToString(move.To()) << ", ";
            }
            std::cout << "\n";
        } else {
            std::cout << "No moves available for Black.\n";
        }
    }

    /*
    // The following sections are commented out but can be enabled for alternative algorithms
    // Example: YBWC and PVS algorithms with varying depths
    // Uncomment and modify as needed
    */

    /*
    if (chessBoard.ColorToMove() == White) {
        std::pair<std::vector<Move>, float> result3 = engine.YBWC<White>(
            chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 7);

        // Check if there is at least one move in the sequence
        if (!result3.first.empty()) {
            Move bestMove = result3.first.front();
            std::cout << "White Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
                      << " with score " << result3.second << "\n";

            // Print the entire sequence of moves
            std::cout << "Best line: ";
            for (const Move &move : result3.first) {
                std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
            }
            std::cout << "\n";
        } else {
            std::cout << "No moves available for White.\n";
        }
    }
    else if (chessBoard.ColorToMove() == Black) {
        std::pair<std::vector<Move>, float> result3 = engine.YBWC<Black>(
            chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 2);

        // Check if there is at least one move in the sequence
        if (!result3.first.empty()) {
            Move bestMove = result3.first.front();
            std::cout << "Black Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
                      << " with score " << result3.second << "\n";

            // Print the entire sequence of moves
            std::cout << "Best line: ";
            for (const Move &move : result3.first) {
                std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
            }
            std::cout << "\n";
        } else {
            std::cout << "No moves available for Black.\n";
        }
    }

    if (chessBoard.ColorToMove() == White) {
        std::pair<std::vector<Move>, float> result3 = engine.PVS<White>(
            chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 7);

        // Check if there is at least one move in the sequence
        if (!result3.first.empty()) {
            Move bestMove = result3.first.front();
            std::cout << "White Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
                      << " with score " << result3.second << "\n";

            // Print the entire sequence of moves
            std::cout << "Best line: ";
            for (const Move &move : result3.first) {
                std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
            }
            std::cout << "\n";
        } else {
            std::cout << "No moves available for White.\n";
        }
    }
    else if (chessBoard.ColorToMove() == Black) {
        std::pair<std::vector<Move>, float> result3 = engine.PVS<Black>(
            chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 2);

        // Check if there is at least one move in the sequence
        if (!result3.first.empty()) {
            Move bestMove = result3.first.front();
            std::cout << "Black Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
                      << " with score " << result3.second << "\n";

            // Print the entire sequence of moves
            std::cout << "Best line: ";
            for (const Move &move : result3.first) {
                std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
            }
            std::cout << "\n";
        } else {
            std::cout << "No moves available for Black.\n";
        }
    }
    */

    return 0;
}
