#ifndef EVALUATION_H
#define EVALUATION_H

#include "./Backend/Board.h"        
#include "./Backend/Type/Piece.h"   
#include "./Backend/Type/Color.h"   
#include <iostream>                 
#include <array>
#include <cassert>

class Evaluation {
public:
    float eval(const StockDory::Board& board) {
        
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

        float materialBalance = static_cast<float>(whiteMaterial - blackMaterial);\

        float castlingBonus = 0.0f;

        const Square WhiteKingInitial = E1; 
        const Square WhiteKingKingside = G1; 
        const Square WhiteKingQueenside = C1;
        const Square WhiteRookKingsideInitial = H1;
        const Square WhiteRookQueensideInitial = A1;
        const Square WhiteRookKingsideCastled = F1; 
        const Square WhiteRookQueensideCastled = D1;

        const Square BlackKingInitial = E8;
        const Square BlackKingKingside = G8;
        const Square BlackKingQueenside = C8;
        const Square BlackRookKingsideInitial = H8;
        const Square BlackRookQueensideInitial = A8;
        const Square BlackRookKingsideCastled = F8; 
        const Square BlackRookQueensideCastled = D8;

        PieceColor whiteKing = board[WhiteKingKingside];
        PieceColor whiteRook = board[WhiteRookKingsideCastled];
        if (whiteKing.Piece() == King && whiteKing.Color() == White &&
            whiteRook.Piece() == Rook && whiteRook.Color() == White) {
            castlingBonus += 1.0f;
            //std::cout << "White has castled kingside. Bonus applied: +1.0" << std::endl;
        }

        whiteKing = board[WhiteKingQueenside];
        whiteRook = board[WhiteRookQueensideCastled];
        if (whiteKing.Piece() == King && whiteKing.Color() == White &&
            whiteRook.Piece() == Rook && whiteRook.Color() == White) {
            castlingBonus += 1.0f;
            //std::cout << "White has castled queenside. Bonus applied: +1.0" << std::endl;
        }

        PieceColor blackKing = board[BlackKingKingside];
        PieceColor blackRook = board[BlackRookKingsideCastled];
        if (blackKing.Piece() == King && blackKing.Color() == Black &&
            blackRook.Piece() == Rook && blackRook.Color() == Black) {
            castlingBonus -= 1.0f;
            //std::cout << "Black has castled kingside. Bonus applied: -1.0" << std::endl;
        }

        blackKing = board[BlackKingQueenside];
        blackRook = board[BlackRookQueensideCastled];
        if (blackKing.Piece() == King && blackKing.Color() == Black &&
            blackRook.Piece() == Rook && blackRook.Color() == Black) {
            castlingBonus -= 1.0f;
            //std::cout << "Black has castled queenside. Bonus applied: -1.0" << std::endl;
        }

        float pawnAdvancementBonus = 0.0f;

        for (int sq = 0; sq < 64; ++sq) {
            PieceColor pc = board[static_cast<Square>(sq)];

            // Process White Pawns
            if (pc.Piece() == Pawn && pc.Color() == White) {
                int currentRank = (sq / 8) + 1; // Ranks 1-8
                int squaresAdvanced = currentRank - 2; // Starting rank is 2
                if (squaresAdvanced > 0) {
                    float bonus = 0.1f * squaresAdvanced;
                    pawnAdvancementBonus += bonus;
                    //std::cout << "White Pawn on square " << sq << " has advanced " << squaresAdvanced 
                    //          << " squares. Bonus applied: +" << bonus << std::endl;
                }
            }

            if (pc.Piece() == Pawn && pc.Color() == Black) {
                int currentRank = (sq / 8) + 1; // Ranks 1-8
                int squaresAdvanced = 7 - currentRank; // Starting rank is 7
                if (squaresAdvanced > 0) {
                    float bonus = 0.1f * squaresAdvanced;
                    pawnAdvancementBonus -= bonus;
                    //std::cout << "Black Pawn on square " << sq << " has advanced " << squaresAdvanced 
                    //          << " squares. Bonus applied: -" << bonus << std::endl;
                }
            }
        }

        materialBalance += castlingBonus;
        materialBalance += pawnAdvancementBonus;

        // Debugging: Print total bonuses and updated material balance
        //std::cout << "Total Castling Bonus: " << castlingBonus << std::endl;
        //std::cout << "Total Pawn Advancement Bonus: " << pawnAdvancementBonus << std::endl;
        //std::cout << "Updated Material Balance: " << materialBalance << std::endl;

        return materialBalance;
    }
};

#endif // EVALUATION_H
