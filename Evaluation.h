// Evaluation.h (Final Corrected Version)
#ifndef EVALUATION_H
#define EVALUATION_H

#include "./Backend/Board.h"       // Ensure correct path based on your project structure
#include "./Backend/Type/Piece.h"  // Include global Piece enum
#include "./Backend/Type/Color.h"  // Include global Color enum
#include <iostream>                // Included for debugging statements
#include <array>
#include <cassert>

class Evaluation {
public:
    // Create evaluation function
    float eval(const StockDory::Board& board) { // Define Board as const StockDory::Board
        // Define material values for each piece type (aligned with Piece enum)
        const std::array<int, 7> PieceValues = {
            1,    // Pawn             // index 0
            3,    // Knight           // index 1
            3,    // Bishop           // index 2
            5,    // Rook             // index 3
            9,    // Queen            // index 4
            0,    // King (not counted in material) // index 5
            0     // NAP (No Piece)   // index 6
        };

        int whiteMaterial = 0;
        int blackMaterial = 0;

        // Initialize loop counter
        int loopCounter = 0;

        // Iterate over all squares to calculate material
        for (int sq = 0; sq < 64; ++sq, ++loopCounter) {
            // Debugging: Print loop counter and current square
            std::cout << "Loop Iteration: " << loopCounter << " - Processing Square: " << sq << std::endl;

            PieceColor pc = board[static_cast<Square>(sq)];

            if (pc.Piece() == NAP || pc.Color() == NAC) {
                continue; // Skip empty squares
            }

            // Retrieve the value of the piece
            int pieceValue = 0;
            if (pc.Piece() >= Pawn && pc.Piece() <= Queen) { // Ensure valid piece type
                pieceValue = PieceValues[static_cast<int>(pc.Piece())];
            }
            else if (pc.Piece() == King) {
                // Debugging: Indicate that a King was encountered and skipped
                std::cout << "Encountered King at square " << sq << ", skipping material count." << std::endl;
                continue; // Skip Kings from material count
            }
            else {
                std::cerr << "Invalid piece type at square " << sq << std::endl;
                continue; // Skip invalid pieces
            }

            // Add to the respective color's material count
            if (pc.Color() == White) {
                whiteMaterial += pieceValue;
                // Debugging: Print current material addition
                std::cout << "Added " << pieceValue << " to White Material. Total White Material: " << whiteMaterial << std::endl;
            }
            else if (pc.Color() == Black) {
                blackMaterial += pieceValue;
                // Debugging: Print current material addition
                std::cout << "Added " << pieceValue << " to Black Material. Total Black Material: " << blackMaterial << std::endl;
            }
            else {
                std::cerr << "Invalid color at square " << sq << std::endl;
            }
        }

        // Calculate material balance
        float materialBalance = static_cast<float>(whiteMaterial - blackMaterial);

        // Debugging statements
        std::cout << "Final White Material: " << whiteMaterial << std::endl;
        std::cout << "Final Black Material: " << blackMaterial << std::endl;
        std::cout << "Final Material Balance (White - Black): " << materialBalance << std::endl;

        return materialBalance;
    }
};

#endif // EVALUATION_H
