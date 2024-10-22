//
// Created by Capks on 10/22/2024.
//

#ifndef ENGINE_H
#define ENGINE_H
#include <limits>

#include "Backend/Board.h"
#include "Backend/Type/Move.h"
#include "Backend/Type/Color.h"
#include "Evaluation.h"
#include <utility>

#include "SimplifiedMoveList.h"

class Engine {
    private:
        Evaluation evaluation;

    public:
        //minimax implementation
        std::pair<Move, float> minimax(StockDory::Board &chessBoard, int depth) {
            //local variable of best move and best score
            Move bestMove;
            float bestScore;
            std::cout << "depth is " << depth;

            //base-case -> when depth is 0, we evaluate the position score and return a default move (which will be overrided in the parent call)
            if (depth == 0) {
                float score = evaluation.eval(chessBoard);
                return std::make_pair(Move(), score);
            }
            //Minimax on white turn -> try to score as high as possible
            else if (chessBoard.ColorToMove() == White) {
                //set best score to negative infinity at at start
                bestScore = -std::numeric_limits<float>::infinity();
                //create move list for white
                const StockDory::SimplifiedMoveList<White> moveList(chessBoard);
                //iterate through the moves and calculate the best score that can be reached from the next position
                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    PreviousState prevState = chessBoard.Move<0>(from, to);
                    std::pair<Move, float> result = minimax(chessBoard, depth-1);
                    //update if we found a better move for white
                    if (bestScore < result.second) {
                        bestScore = result.second;
                        bestMove = nextMove;
                    }
                    chessBoard.UndoMove<0>(prevState, from, to);
                }
            }
            //Minimax on black turn -> try to score as low as possible
            else {
                //set best score to positive infinity
                bestScore = std::numeric_limits<float>::infinity();
                //calculate possible moves for black
                const StockDory::SimplifiedMoveList<Black> moveList(chessBoard);
                //repeat the same thing as white but for black instead
                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    PreviousState prevState = chessBoard.Move<0>(from, to);
                    std::pair<Move, float> result = minimax(chessBoard, depth-1);
                    if (bestScore > result.second) {
                        bestScore = result.second;
                        bestMove = nextMove;
                    }
                    chessBoard.UndoMove<0>(prevState, from, to);
                }
            }
            return std::make_pair(bestMove, bestScore);
        }
};

#endif //ENGINE_H
