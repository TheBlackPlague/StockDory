// Evaluation.h (New Version - Evaluation Function with Move-Specific Bonuses)
#ifndef EVALUATION_H
#define EVALUATION_H

#include "./Backend/Board.h"        // Ensure correct path based on your project structure
#include "./Backend/Type/Piece.h"   // Include global Piece enum
#include "./Backend/Type/Color.h"   // Include global Color enum
#include <iostream>                 // Included for debugging statements
#include <array>
#include <cassert>

class Evaluation {
public:
    float eval(const StockDory::Board& board) { // Pass by const StockDory::Board reference
        
        // Define material values for each piece type (aligned with Piece enum)
        const std::array<int, 7> PieceValues = {
            0,    // Pawn             // index 0
            0,    // Knight           // index 1
            0,    // Bishop           // index 2
            0,    // Rook             // index 3
            0,    // Queen            // index 4
            0,    // King (not counted in material) // index 5
            0     // NAP (No Piece)   // index 6
        };

        int whiteMaterial = 0;
        int blackMaterial = 0;
        int loopCounter = 0;

        // Iterate over all squares to calculate material
        for (int sq = 0; sq < 64; ++sq, ++loopCounter) {
            // Debugging: Print loop counter and current square
            // std::cout << "Loop Iteration: " << loopCounter << " - Processing Square: " << sq << std::endl;

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
                // std::cout << "Encountered King at square " << sq << ", skipping material count." << std::endl;
                continue; // Skip Kings from material count
            }
            else {
                std::cerr << "Invalid piece type at square " << sq << std::endl;
                continue; // Skip invalid pieces
            }

            // Add to the respective color's material count
            if (pc.Color() == White) {
                whiteMaterial += pieceValue;
                // std::cout << "Added " << pieceValue << " to White Material. Total White Material: " << whiteMaterial << std::endl;
            }
            else if (pc.Color() == Black) {
                blackMaterial += pieceValue;
                // std::cout << "Added " << pieceValue << " to Black Material. Total Black Material: " << blackMaterial << std::endl;
            }
            else {
                std::cerr << "Invalid color at square " << sq << std::endl;
            }
        }

        float materialBalance = static_cast<float>(whiteMaterial - blackMaterial);

        // Define specific squares for move-based bonuses
        const Square WhitePawnE4 = E4; // e4
        const Square BlackPawnE5 = E5; // e5
        const Square WhitePawnC4 = C4; // c4
        const Square BlackPawnC5 = C5; // c5
        const Square WhiteKnightF3 = F3; // f3
        const Square BlackKnightF6 = F6; // f6
        const Square WhiteKnightC3 = C3; // c3
        const Square BlackKnightC6 = C6; // c6

        // Initialize bonus variable
        float moveBonus = 0.0f;

        // Check for White Pawn to e4
        PieceColor piece = board[WhitePawnE4];
        if (piece.Piece() == Pawn && piece.Color() == White) {
            moveBonus += 0.4f;
            //std::cout << "White Pawn is on e4. Bonus applied: +0.4" << std::endl;
        }

        // Check for Black Pawn to e5
        piece = board[BlackPawnE5];
        if (piece.Piece() == Pawn && piece.Color() == Black) {
            moveBonus -= 0.4f;
            //std::cout << "Black Pawn is on e5. Bonus applied: -0.4" << std::endl;
        }

        // Check for White Pawn to c4
        piece = board[WhitePawnC4];
        if (piece.Piece() == Pawn && piece.Color() == White) {
            moveBonus += 0.3f;
            //std::cout << "White Pawn is on c4. Bonus applied: +0.3" << std::endl;
        }

        // Check for Black Pawn to c5
        piece = board[BlackPawnC5];
        if (piece.Piece() == Pawn && piece.Color() == Black) {
            moveBonus -= 0.3f;
            //std::cout << "Black Pawn is on c5. Bonus applied: -0.3" << std::endl;
        }

        // Check for White Knight to f3
        piece = board[WhiteKnightF3];
        if (piece.Piece() == Knight && piece.Color() == White) {
            moveBonus += 0.2f;
            //std::cout << "White Knight is on f3. Bonus applied: +0.2" << std::endl;
        }

        // Check for Black Knight to f6
        piece = board[BlackKnightF6];
        if (piece.Piece() == Knight && piece.Color() == Black) {
            moveBonus -= 0.2f;
            //std::cout << "Black Knight is on f6. Bonus applied: -0.2" << std::endl;
        }

        // Check for White Knight to c3
        piece = board[WhiteKnightC3];
        if (piece.Piece() == Knight && piece.Color() == White) {
            moveBonus += 0.1f;
            //std::cout << "White Knight is on c3. Bonus applied: +0.1" << std::endl;
        }

        // Check for Black Knight to c6
        piece = board[BlackKnightC6];
        if (piece.Piece() == Knight && piece.Color() == Black) {
            moveBonus -= 0.1f;
            //std::cout << "Black Knight is on c6. Bonus applied: -0.1" << std::endl;
        }

        materialBalance += moveBonus;

        //std::cout << "Total Move Bonuses Applied: " << moveBonus << std::endl;
        //std::cout << "Updated Material Balance: " << materialBalance << std::endl;

        return materialBalance;
    }
};

#endif // EVALUATION_H
