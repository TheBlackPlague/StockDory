#include <iostream>
#include <limits>
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

    Engine engine;
    // std::pair<Move, float> result = engine.minimax(chessBoard, 3);
    // std::cout << "Result is: " << squareToString(result.first.From()) << " to " << squareToString(result.first.To()) << " with score " << result.second << "\n";
    //
    // std::pair<Move, float> result2 = engine.alphaBeta(chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 3);
    // std::cout << "Result is: " << squareToString(result2.first.From()) << " to " << squareToString(result2.first.To()) << " with score " << result2.second << "\n"
    if (chessBoard.ColorToMove() == White) {
        std::pair<std::vector<Move>, float> result3 = engine.alphaBetaNega<White>(
            chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 1);

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
        std::pair<std::vector<Move>, float> result3 = engine.alphaBetaNega<Black>(
            chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 1);

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

    // if (chessBoard.ColorToMove() == White) {
    //     std::pair<std::vector<Move>, float> result3 = engine.YBWC<White>(
    //         chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 7);
    //
    //     // Check if there is at least one move in the sequence
    //     if (!result3.first.empty()) {
    //         Move bestMove = result3.first.front();
    //         std::cout << "White Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
    //                   << " with score " << result3.second << "\n";
    //
    //         // Print the entire sequence of moves
    //         std::cout << "Best line: ";
    //         for (const Move &move : result3.first) {
    //             std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
    //         }
    //         std::cout << "\n";
    //     } else {
    //         std::cout << "No moves available for White.\n";
    //     }
    // }
    // else if (chessBoard.ColorToMove() == Black) {
    //     std::pair<std::vector<Move>, float> result3 = engine.YBWC<Black>(
    //         chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 2);
    //
    //     // Check if there is at least one move in the sequence
    //     if (!result3.first.empty()) {
    //         Move bestMove = result3.first.front();
    //         std::cout << "Black Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
    //                   << " with score " << result3.second << "\n";
    //
    //         // Print the entire sequence of moves
    //         std::cout << "Best line: ";
    //         for (const Move &move : result3.first) {
    //             std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
    //         }
    //         std::cout << "\n";
    //     } else {
    //         std::cout << "No moves available for Black.\n";
    //     }
    // }
    //
    // if (chessBoard.ColorToMove() == White) {
    //     std::pair<std::vector<Move>, float> result3 = engine.PVS<White>(
    //         chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 7);
    //
    //     // Check if there is at least one move in the sequence
    //     if (!result3.first.empty()) {
    //         Move bestMove = result3.first.front();
    //         std::cout << "White Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
    //                   << " with score " << result3.second << "\n";
    //
    //         // Print the entire sequence of moves
    //         std::cout << "Best line: ";
    //         for (const Move &move : result3.first) {
    //             std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
    //         }
    //         std::cout << "\n";
    //     } else {
    //         std::cout << "No moves available for White.\n";
    //     }
    // }
    // else if (chessBoard.ColorToMove() == Black) {
    //     std::pair<std::vector<Move>, float> result3 = engine.PVS<Black>(
    //         chessBoard, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 2);
    //
    //     // Check if there is at least one move in the sequence
    //     if (!result3.first.empty()) {
    //         Move bestMove = result3.first.front();
    //         std::cout << "Black Result is: " << squareToString(bestMove.From()) << " to " << squareToString(bestMove.To())
    //                   << " with score " << result3.second << "\n";
    //
    //         // Print the entire sequence of moves
    //         std::cout << "Best line: ";
    //         for (const Move &move : result3.first) {
    //             std::cout << squareToString(move.From()) << " to " << squareToString(move.To()) << ", ";
    //         }
    //         std::cout << "\n";
    //     } else {
    //         std::cout << "No moves available for Black.\n";
    //     }
    // }

    return 0;
}
