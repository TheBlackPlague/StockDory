// Evaluation.h
#ifndef EVALUATION_H
#define EVALUATION_H

#include "Backend/Board.h" // Ensure this path is correct based on your project structure
#include <cstdlib>
#include <iostream> // Included for debugging statements

class Evaluation {
public:
    // Create evaluation function
    float eval(const StockDory::Board& board) { // Changed parameter to const reference for efficiency
        // Define material values for each piece type (in arbitrary units)
        const std::array<int, 7> PieceValues = {
            0,    // NAP (No Piece)
            1,    // Pawn
            3,    // Knight
            3,    // Bishop
            5,    // Rook
            9,    // Queen
            0     // King (not counted in material)
        };

        int whiteMaterial = 0;
        int blackMaterial = 0;

        // Iterate over all squares to calculate material
        for (int sq = 0; sq < 64; ++sq) {
            PieceColor pc = board[static_cast<Square>(sq)];
            
            if (pc.Piece() == NAP || pc.Color() == NAC) {
                continue; // Skip empty squares
            }

            // Retrieve the value of the piece
            int pieceValue = 0;
            if (pc.Piece() >= Pawn && pc.Piece() <= Queen) { // Ensure valid piece type
                pieceValue = PieceValues[static_cast<int>(pc.Piece())];
            } else {
                std::cerr << "Invalid piece type at square " << sq << std::endl;
                continue; // Skip invalid pieces
            }

            // Add to the respective color's material count
            if (pc.Color() == White) {
                whiteMaterial += pieceValue;
            } else if (pc.Color() == Black) {
                blackMaterial += pieceValue;
            } else {
                std::cerr << "Invalid color at square " << sq << std::endl;
            }
        }

        // Calculate material balance
        float materialBalance = static_cast<float>(whiteMaterial - blackMaterial);

        // Debugging statements
        std::cout << "White Material: " << whiteMaterial << std::endl;
        std::cout << "Black Material: " << blackMaterial << std::endl;
        std::cout << "Material Balance (White - Black): " << materialBalance << std::endl;

        return materialBalance;
    }
};

#endif // EVALUATION_H
