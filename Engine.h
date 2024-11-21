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
#include <omp.h>


#include "SimplifiedMoveList.h"

class Engine {
    private:
        Evaluation evaluation;
        int numThreads = 8;
        int mateScore = 20000;

    public:
        template<Color color>
        int minimaxMoveCounter(StockDory::Board &chessBoard, int depth) {
            int sum = 0;
            if (chessBoard.ColorToMove() == White) {
                const StockDory::SimplifiedMoveList<White> moveList(chessBoard);

                // Add check for no legal moves
                if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                    return 0;
                }
                else if (moveList.Count() == 0) {
                    // No legal moves
                    int score = evaluation.eval(chessBoard);
                    return 0;
                }

                if (depth == 0) {
                    int score = evaluation.eval(chessBoard);
                    return 0;
                }
                constexpr Color Ocolor = Opposite(color);
                sum += moveList.Count();
                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    Piece promotion = nextMove.Promotion();
                    // Perform move
                    PreviousState prevState = chessBoard.Move<0>(from, to, promotion);
                    sum += minimaxMoveCounter<Ocolor>(chessBoard, depth-1);
                    // Undo move
                    chessBoard.UndoMove<0>(prevState, from, to);
                }
            }
            // Black's turn
            else {
                const StockDory::SimplifiedMoveList<Black> moveList(chessBoard);
                // Add check for no legal moves
                if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                    return 0;
                }
                else if (moveList.Count() == 0) {
                    // No legal moves
                    int score = evaluation.eval(chessBoard);
                    return 0;
                }

                if (depth == 0) {
                    int score = evaluation.eval(chessBoard);
                    return 0;
                }
                constexpr Color Ocolor = Opposite(color);
                sum += moveList.Count();
                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    Piece promotion = nextMove.Promotion();
                    // Perform move
                    PreviousState prevState = chessBoard.Move<0>(from, to, promotion);
                    sum += minimaxMoveCounter<Ocolor>(chessBoard, depth-1);
                    // Undo move
                    chessBoard.UndoMove<0>(prevState, from, to);
                }
            }
            return sum;
        }
        //minimax implementation
        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> minimax(StockDory::Board &chessBoard, int depth) {
            std::array<Move, maxDepth> bestLine;
            int bestScore;
            int bestLineSize;
            // Base case

            // White's turn
            if (chessBoard.ColorToMove() == White) {
                bestScore = -50000;
                const StockDory::SimplifiedMoveList<White> moveList(chessBoard);

                // Add check for no legal moves
                if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                    return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
                }
                else if (moveList.Count() == 0) {
                    // No legal moves
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), 0);
                }

                if (depth == 0) {
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), score);
                }
                constexpr Color Ocolor = Opposite(color);

                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    Piece promotion = nextMove.Promotion();
                    // Perform move
                    PreviousState prevState = chessBoard.Move<0>(from, to, promotion);
                    std::pair<std::array<Move, maxDepth>, int> result = minimax<Ocolor, maxDepth>(chessBoard, depth-1);
                    // Update best score
                    if (bestScore < result.second) {
                        bestScore = result.second;
                        bestLine[0] = nextMove;
                        bestLineSize = 1;
                        for (int j = 0; j < depth - 1; j++) {
                            bestLine[bestLineSize++] = result.first[j];
                        }
                    }
                    // Undo move
                    chessBoard.UndoMove<0>(prevState, from, to);
                }
            }
            // Black's turn
            else {
                bestScore = 50000;
                const StockDory::SimplifiedMoveList<Black> moveList(chessBoard);

                // Add check for no legal moves
                if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                    return std::make_pair(std::array<Move, maxDepth>(), mateScore+depth);
                }
                else if (moveList.Count() == 0) {
                    // No legal moves
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), 0);
                }

                if (depth == 0) {
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), score);
                }
                constexpr Color Ocolor = Opposite(color);

                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    Piece promotion = nextMove.Promotion();
                    // Perform move
                    PreviousState prevState = chessBoard.Move<0>(from, to, promotion);
                    std::pair<std::array<Move, maxDepth>, int> result = minimax<Ocolor, maxDepth>(chessBoard, depth-1);
                    // Update best score
                    if (bestScore > result.second) {
                        bestScore = result.second;
                        bestLine[0] = nextMove;
                        bestLineSize = 1;
                        for (int j = 0; j < depth - 1; j++) {
                            bestLine[bestLineSize++] = result.first[j];
                        }
                    }
                    // Undo move
                    chessBoard.UndoMove<0>(prevState, from, to);
                }
            }

            return std::make_pair(bestLine, bestScore);
        }

        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> parallelMinimax(StockDory::Board &chessBoard, int depth) {
            // Local variables
            std::array<Move, maxDepth> bestLine;
            int bestScore;
            int bestLineSize;
            // Base case

            // White's turn
            if (chessBoard.ColorToMove() == White) {
                bestScore = -50000;
                const StockDory::SimplifiedMoveList<White> moveList(chessBoard);

                // Add check for no legal moves
                if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                    return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
                }
                else if (moveList.Count() == 0) {
                    // No legal moves
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), 0);
                }

                if (depth == 0) {
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), score);
                }
                constexpr Color Ocolor = Opposite(color);
#pragma omp parallel for schedule(dynamic)
                for (uint8_t i = 0; i < moveList.Count(); i++) {;
                    StockDory::Board localBoard = chessBoard;
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    Piece promotion = nextMove.Promotion();
                    // Perform move
                    PreviousState prevState = localBoard.Move<0>(from, to, promotion);
                    std::pair<std::array<Move, maxDepth>, int> result = minimax<Ocolor, maxDepth>(localBoard, depth-1);
                    // Update best score
#pragma omp critical
                    {
                        if (bestScore < result.second) {
                            bestScore = result.second;
                            bestLine[0] = nextMove;
                            bestLineSize = 1;
                            for (int j = 0; j < depth - 1; j++) {
                                bestLine[bestLineSize++] = result.first[j];
                            }
                        }
                    }
                    // Undo move
                    localBoard.UndoMove<0>(prevState, from, to);
                }
            }
            // Black's turn
            else {
                bestScore = 50000;
                const StockDory::SimplifiedMoveList<Black> moveList(chessBoard);

                // Add check for no legal moves
                if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                    return std::make_pair(std::array<Move, maxDepth>(), mateScore+depth);
                }
                else if (moveList.Count() == 0) {
                    // No legal moves
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), 0);
                }

                if (depth == 0) {
                    int score = evaluation.eval(chessBoard);
                    return std::make_pair(std::array<Move, maxDepth>(), score);
                }
                constexpr Color Ocolor = Opposite(color);
#pragma omp parallel for schedule(dynamic)
                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    StockDory::Board localBoard = chessBoard;
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    Piece promotion = nextMove.Promotion();
                    // Perform move
                    PreviousState prevState = localBoard.Move<0>(from, to, promotion);
                    std::pair<std::array<Move, maxDepth>, int> result = minimax<Ocolor, maxDepth>(localBoard, depth-1);
                    // Update best score
                    #pragma omp critical
                    {
                        if (bestScore > result.second) {
                            bestScore = result.second;
                            bestLine[0] = nextMove;
                            bestLineSize = 1;
                            for (int j = 0; j < depth - 1; j++) {
                                bestLine[bestLineSize++] = result.first[j];
                            }
                        }
                    }
                    // Undo move
                    localBoard.UndoMove<0>(prevState, from, to);
                }
            }

            return std::make_pair(bestLine, bestScore);
        }

        std::pair<Move, int> alphaBeta(StockDory::Board &chessBoard, int alpha, int beta, int depth) {
            //local variable of best move and best score
            Move bestMove;
            int bestScore;

            //base-case -> when depth is 0, we evaluate the position score and return a default move (which will be overrided in the parent call)
            if (depth == 0) {
                int score = evaluation.eval(chessBoard);
                return std::make_pair(Move(), score);
            }
            //Minimax on white turn -> try to score as high as possible
            else if (chessBoard.ColorToMove() == White) {
                //set best score to negative infinity at start
                bestScore = -50000;
                //create move list for white
                const StockDory::SimplifiedMoveList<White> moveList(chessBoard);
                //iterate through the moves and calculate the best score that can be reached from the next position
                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    //Perform move
                    PreviousState prevState = chessBoard.Move<0>(from, to);
                    std::pair<Move, int> result = alphaBeta(chessBoard, alpha, beta, depth-1);
                    //update if we found a better move for white
                    if (bestScore < result.second) {
                        bestScore = result.second;
                        bestMove = nextMove;
                    }
                    //Undo move
                    chessBoard.UndoMove<0>(prevState, from, to);
                    //alpha check
                    alpha = std::max(alpha, result.second);
                    if (beta <= alpha) {
                        break;
                    }

                }
            }
            //Minimax on black turn -> try to score as low as possible
            else {
                //set best score to positive infinity
                bestScore = 50000;
                //calculate possible moves for black
                const StockDory::SimplifiedMoveList<Black> moveList(chessBoard);
                //repeat the same thing as white but for black instead
                for (uint8_t i = 0; i < moveList.Count(); i++) {
                    Move nextMove = moveList[i];
                    Square from = nextMove.From();
                    Square to = nextMove.To();
                    Piece promotion = nextMove.Promotion();
                    //Perform move
                    PreviousState prevState = chessBoard.Move<0>(from, to, promotion);
                    std::pair<Move, int> result = alphaBeta(chessBoard, alpha, beta, depth-1);
                    if (bestScore > result.second) {
                        bestScore = result.second;
                        bestMove = nextMove;
                    }
                    //Undo move
                    chessBoard.UndoMove<0>(prevState, from, to);
                    //beta check
                    beta = std::min(beta, result.second);
                    if (beta <= alpha) {
                        break;
                    }
                }
            }
            return std::make_pair(bestMove, bestScore);
        }

        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> alphaBetaNegaMoveCounter(StockDory::Board &chessBoard, int alpha, int beta, int depth, int &count) {
             //local variable of best line and best score
             int bestScore;
             std::array<Move, maxDepth> bestLine;
             int bestLineSize;
             //create move list for player
             const StockDory::SimplifiedMoveList<color> moveList(chessBoard);
             //check for mate
             if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                 return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
             }
             //stalemate
             else if (moveList.Count() == 0){
                 return std::make_pair(std::array<Move, maxDepth>(), 0);
             }
             //base-case -> when depth is 0, we evaluate the position score and return a default move (which will be overrided in the parent call)
             if (depth == 0) {
                 int score = evaluation.eval(chessBoard);
                 //flip the score for black since we are maximizing
                 if (color == Black) {
                     score *= -1;
                 }
                 return std::make_pair(std::array<Move, maxDepth>(), score);
             }
             constexpr enum Color Ocolor = Opposite(color);
             //Assume from one perspective they are always the maximizer
             //Set best score to negative infinity at start
             bestScore = -50000;
             //iterate through the moves and calculate the best score that can be reached from the next position
             for (uint8_t i = 0; i < moveList.Count(); i++) {
                 count++;
                 Move nextMove = moveList[i];
                 Square from = nextMove.From();
                 Square to = nextMove.To();
                 Piece promotion = nextMove.Promotion();
                 //Perform move
                 PreviousState prevState = chessBoard.Move<0>(from, to, promotion);
                 std::pair<std::array<Move, maxDepth>, int> result = alphaBetaNegaMoveCounter<Ocolor, maxDepth>(chessBoard, -beta, -alpha, depth-1, count);
                 //update if we found a better move for white
                 result.second = -result.second;
                 if (bestScore < result.second) {
                     //create chess line with the current move as the head
                     bestLine[0] = nextMove;
                     //append result line to the current move
                     bestLineSize = 1;
                     for (int j = 0; j < depth - 1; j++) {
                         bestLine[bestLineSize++] = result.first[j];
                     }
                     bestScore = result.second;
                 }
                 //Undo move
                 chessBoard.UndoMove<0>(prevState, from, to);
                 //alpha check
                 alpha = std::max(alpha, result.second);
                 if (beta <= alpha) {
                     break;
                 }
             }

             return std::make_pair(bestLine, bestScore);
         }

        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> alphaBetaNega(StockDory::Board &chessBoard, int alpha, int beta, int depth) {
             //local variable of best line and best score
             int bestScore;
             std::array<Move, maxDepth> bestLine;
             int bestLineSize;
             //create move list for player
             const StockDory::SimplifiedMoveList<color> moveList(chessBoard);
             //check for mate
             if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                 return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
             }
             //stalemate
             else if (moveList.Count() == 0){
                 return std::make_pair(std::array<Move, maxDepth>(), 0);
             }
             //base-case -> when depth is 0, we evaluate the position score and return a default move (which will be overrided in the parent call)
             if (depth == 0) {
                 int score = evaluation.eval(chessBoard);
                 //flip the score for black since we are maximizing
                 if (color == Black) {
                     score *= -1;
                 }
                 return std::make_pair(std::array<Move, maxDepth>(), score);
             }
             constexpr enum Color Ocolor = Opposite(color);
             //Assume from one perspective they are always the maximizer
             //Set best score to negative infinity at start
             bestScore = -50000;
             //iterate through the moves and calculate the best score that can be reached from the next position
             for (uint8_t i = 0; i < moveList.Count(); i++) {
                 Move nextMove = moveList[i];
                 Square from = nextMove.From();
                 Square to = nextMove.To();
                 Piece promotion = nextMove.Promotion();
                 //Perform move
                 PreviousState prevState = chessBoard.Move<0>(from, to, promotion);
                 std::pair<std::array<Move, maxDepth>, int> result = alphaBetaNega<Ocolor, maxDepth>(chessBoard, -beta, -alpha, depth-1);
                 //update if we found a better move for white
                 result.second = -result.second;
                 if (bestScore < result.second) {
                     //create chess line with the current move as the head
                     bestLine[0] = nextMove;
                     //append result line to the current move
                     bestLineSize = 1;
                     for (int j = 0; j < depth - 1; j++) {
                         bestLine[bestLineSize++] = result.first[j];
                     }
                     bestScore = result.second;
                 }
                 //Undo move
                 chessBoard.UndoMove<0>(prevState, from, to);
                 //alpha check
                 alpha = std::max(alpha, result.second);
                 if (beta <= alpha) {
                     break;
                 }
             }

             return std::make_pair(bestLine, bestScore);
         }
    
        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> naiveParallelAlphaBeta(const StockDory::Board &chessBoard, int alpha, int beta, int depth) {
            std::array<Move, maxDepth> bestLine;
            int bestScore = -50000;
            // create move list for player
            const StockDory::SimplifiedMoveList<color> moveList(chessBoard);
             //check for mate
            if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
            }
            //stalemate
            else if (moveList.Count() == 0){
                return std::make_pair(std::array<Move, maxDepth>(), 0);
            }
            if (depth == 0) {
                int score = evaluation.eval(chessBoard);
                if (color == Black) {
                    score *= -1;
                }
                return std::make_pair(std::array<Move, maxDepth>(), score);
            }

            constexpr enum Color Ocolor = Opposite(color);

            //Dynamic schedule since we do not know the ordering of moves or the number of moves in each call
            #pragma omp parallel for shared(alpha, beta) schedule(dynamic)
            for (uint8_t i = 0; i < moveList.Count(); i++) {
                // int thread = omp_get_thread_num();
                // // printf("%d\n", thread);
                // printf("I hit the for loop for thread %d \n", thread);
                if (alpha >= beta) {
                    continue; // Mimic cutoff because you cannot break in
                }
                //Private copy of the board for each thread
                StockDory::Board threadBoard = chessBoard;
                Move nextMove = moveList[i];
                Square from = nextMove.From();
                Square to = nextMove.To();
                Piece promotion = nextMove.Promotion();
                PreviousState prevState = threadBoard.Move<0>(from, to, promotion);
                std::pair<std::array<Move, maxDepth>, int> localResult = alphaBetaNega<Ocolor, maxDepth>(threadBoard, -beta, -alpha, depth - 1);
                localResult.second = -localResult.second;
                threadBoard.UndoMove<0>(prevState, from, to);
                #pragma omp critical
                {
                    if (localResult.second > bestScore) {
                        bestScore = localResult.second;
                        bestLine[0] = nextMove;
                        for (int j = 0; j < depth - 1; j++) {
                            bestLine[j + 1] = localResult.first[j];
                        }
                        alpha = std::max(alpha, bestScore);
                    }
                }
            }

            return std::make_pair(bestLine, bestScore);
        }

        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> naiveParallelYBAlphaBeta(const StockDory::Board &chessBoard, int alpha, int beta, int depth) {
            std::array<Move, maxDepth> bestLine;
            int bestScore = -50000;
            // create move list for player
            const StockDory::SimplifiedMoveList<color> moveList(chessBoard);
             //check for mate
            if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
            }
            //stalemate
            else if (moveList.Count() == 0){
                return std::make_pair(std::array<Move, maxDepth>(), 0);
            }
            if (depth == 0) {
                int score = evaluation.eval(chessBoard);
                if (color == Black) {
                    score *= -1;
                }
                return std::make_pair(std::array<Move, maxDepth>(), score);
            }

            constexpr enum Color Ocolor = Opposite(color);

            // Process the leftmost child sequentially
            Move PV = moveList[0];
            Square from = PV.From();
            Square to = PV.To();
            Piece promotion = PV.Promotion();
            //create local copy for safety
            StockDory::Board boardCopy = chessBoard;
            PreviousState prevState = boardCopy.Move<0>(from, to, promotion);
            std::pair<std::array<Move, maxDepth>, int> result = naiveParallelYBAlphaBeta<Ocolor, maxDepth>(boardCopy, -beta, -alpha, depth - 1);
            result.second = -result.second;
            boardCopy.UndoMove<0>(prevState, from, to);
            if (result.second > bestScore) {
                bestScore = result.second;
                bestLine[0] = PV;
                //Store best line
                for (int j = 0; j < depth - 1; j++) {
                    bestLine[j + 1] = result.first[j];
                }
                alpha = std::max(alpha, bestScore);
            }
            //Cutoff
            if (alpha >= beta) {
                return std::make_pair(bestLine, bestScore);
            }
            //Dynamic schedule since we do not know the ordering of moves or the number of moves in each call
            #pragma omp parallel for shared(alpha, beta) schedule(dynamic)
            for (uint8_t i = 1; i < moveList.Count(); i++) {
                // int thread = omp_get_thread_num();
                // // printf("%d\n", thread);
                // printf("I hit the for loop for thread %d \n", thread);
                if (alpha >= beta) {
                    continue; // Mimic cutoff because you cannot break in
                }
                //Private copy of the board for each thread
                StockDory::Board threadBoard = chessBoard;
                Move nextMove = moveList[i];
                Square from = nextMove.From();
                Square to = nextMove.To();
                Piece promotion = nextMove.Promotion();
                PreviousState prevState = threadBoard.Move<0>(from, to, promotion);
                std::pair<std::array<Move, maxDepth>, int> localResult = alphaBetaNega<Ocolor, maxDepth>(threadBoard, -beta, -alpha, depth - 1);
                localResult.second = -localResult.second;
                threadBoard.UndoMove<0>(prevState, from, to);
                #pragma omp critical
                {
                    if (localResult.second > bestScore) {
                        bestScore = localResult.second;
                        bestLine[0] = nextMove;
                        for (int j = 0; j < depth - 1; j++) {
                            bestLine[j + 1] = localResult.first[j];
                        }
                        alpha = std::max(alpha, bestScore);
                    }
                }
            }

            return std::make_pair(bestLine, bestScore);
        }

        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> YBWC(const StockDory::Board &chessBoard, int alpha, int beta, int depth) {
            std::array<Move, maxDepth> bestLine;
            int bestScore = -50000;
            // create move list for player
            const StockDory::SimplifiedMoveList<color> moveList(chessBoard);
             //check for mate
            if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
            }
            //stalemate
            else if (moveList.Count() == 0){
                return std::make_pair(std::array<Move, maxDepth>(), 0);
            }
            if (depth == 0) {
                int score = evaluation.eval(chessBoard);
                if (color == Black) {
                    score *= -1;
                }
                return std::make_pair(std::array<Move, maxDepth>(), score);
            }

            constexpr enum Color Ocolor = Opposite(color);

            // Process the leftmost child sequentially
            Move PV = moveList[0];
            Square from = PV.From();
            Square to = PV.To();
            Piece promotion = PV.Promotion();
            //create local copy for safety
            StockDory::Board boardCopy = chessBoard;
            PreviousState prevState = boardCopy.Move<0>(from, to, promotion);
            std::pair<std::array<Move, maxDepth>, int> result = YBWC<Ocolor, maxDepth>(boardCopy, -beta, -alpha, depth - 1);
            result.second = -result.second;
            boardCopy.UndoMove<0>(prevState, from, to);
            if (result.second > bestScore) {
                bestScore = result.second;
                bestLine[0] = PV;
                //Store best line
                for (int j = 0; j < depth - 1; j++) {
                    bestLine[j + 1] = result.first[j];
                }
                alpha = std::max(alpha, bestScore);
            }
            //Cutoff
            if (alpha >= beta) {
                return std::make_pair(bestLine, bestScore);
            }
            //Dynamic schedule since we do not know the ordering of moves or the number of moves in each call
            #pragma omp parallel for shared(alpha, beta) schedule(dynamic)
            for (uint8_t i = 1; i < moveList.Count(); i++) {
                // int thread = omp_get_thread_num();
                // // printf("%d\n", thread);
                // printf("I hit the for loop for thread %d \n", thread);
                if (alpha >= beta) {
                    continue; // Mimic cutoff because you cannot break in
                }
                //Private copy of the board for each thread
                StockDory::Board threadBoard = chessBoard;
                Move nextMove = moveList[i];
                Square from = nextMove.From();
                Square to = nextMove.To();
                Piece promotion = nextMove.Promotion();
                PreviousState prevState = threadBoard.Move<0>(from, to, promotion);
                std::pair<std::array<Move, maxDepth>, int> localResult = YBWC<Ocolor, maxDepth>(threadBoard, -beta, -alpha, depth - 1);
                localResult.second = -localResult.second;
                threadBoard.UndoMove<0>(prevState, from, to);
                #pragma omp critical
                {
                    if (localResult.second > bestScore) {
                        bestScore = localResult.second;
                        bestLine[0] = nextMove;
                        for (int j = 0; j < depth - 1; j++) {
                            bestLine[j + 1] = localResult.first[j];
                        }
                        alpha = std::max(alpha, bestScore);
                    }
                }
            }

            return std::make_pair(bestLine, bestScore);
        }

        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> PVS(const StockDory::Board &chessBoard, int alpha, int beta, int depth) {
            std::array<Move, maxDepth> bestLine;
            int bestScore = -50000;
            // create move list for player
            const StockDory::SimplifiedMoveList<color> moveList(chessBoard);
             //check for mate
            if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
            }
            //stalemate
            else if (moveList.Count() == 0){
                return std::make_pair(std::array<Move, maxDepth>(), 0);
            }
            if (depth == 0) {
                int score = evaluation.eval(chessBoard);
                if (color == Black) {
                    score *= -1;
                }
                return std::make_pair(std::array<Move, maxDepth>(), score);
            }

            constexpr enum Color Ocolor = Opposite(color);

            // Process the leftmost child sequentially
            Move PV = moveList[0];
            Square from = PV.From();
            Square to = PV.To();
            Piece promotion = PV.Promotion();
            //create local copy for safety
            StockDory::Board boardCopy = chessBoard;
            PreviousState prevState = boardCopy.Move<0>(from, to, promotion);
            std::pair<std::array<Move, maxDepth>, int> result = PVS<Ocolor, maxDepth>(boardCopy, -beta, -alpha, depth - 1);
            result.second = -result.second;
            boardCopy.UndoMove<0>(prevState, from, to);
            if (result.second > bestScore) {
                bestScore = result.second;
                bestLine[0] = PV;
                //Store best line
                for (int j = 0; j < depth - 1; j++) {
                    bestLine[j + 1] = result.first[j];
                }
                alpha = std::max(alpha, bestScore);
            }
            //Cutoff
            if (alpha >= beta) {
                return std::make_pair(bestLine, bestScore);
            }
            //Dynamic schedule since we do not know the ordering of moves or the number of moves in each call
            #pragma omp parallel for shared(alpha, beta) schedule(dynamic)
            for (uint8_t i = 1; i < moveList.Count(); i++) {
                // int thread = omp_get_thread_num();
                // // printf("%d\n", thread);
                // printf("I hit the for loop for thread %d \n", thread);
                if (alpha >= beta) {
                    continue; // Mimic cutoff because you cannot break in
                }
                //Private copy of the board for each thread
                StockDory::Board threadBoard = chessBoard;
                Move nextMove = moveList[i];
                Square from = nextMove.From();
                Square to = nextMove.To();
                Piece promotion = nextMove.Promotion();
                PreviousState prevState = threadBoard.Move<0>(from, to, promotion);
                std::pair<std::array<Move, maxDepth>, int> localResult = alphaBetaNegaParallel<Ocolor, maxDepth>(threadBoard, -beta, -alpha, depth - 1);
                localResult.second = -localResult.second;
                threadBoard.UndoMove<0>(prevState, from, to);
                #pragma omp critical
                {
                    if (localResult.second > bestScore) {
                        bestScore = localResult.second;
                        bestLine[0] = nextMove;
                        for (int j = 0; j < depth - 1; j++) {
                            bestLine[j + 1] = localResult.first[j];
                        }
                        alpha = std::max(alpha, bestScore);
                    }
                }
            }

            return std::make_pair(bestLine, bestScore);
        }

        template<Color color, int maxDepth>
        std::pair<std::array<Move, maxDepth>, int> alphaBetaNegaParallel(const StockDory::Board &chessBoard, int alpha, int beta, int depth) {
            std::array<Move, maxDepth> bestLine;
            int bestScore = -50000;
            // create move list for player
            const StockDory::SimplifiedMoveList<color> moveList(chessBoard);
             //check for mate
            if (moveList.Count() == 0 and chessBoard.Checked<color>()) {
                return std::make_pair(std::array<Move, maxDepth>(), -mateScore-depth);
            }
            //stalemate
            else if (moveList.Count() == 0){
                return std::make_pair(std::array<Move, maxDepth>(), 0);
            }
            if (depth == 0) {
                int score = evaluation.eval(chessBoard);
                if (color == Black) {
                    score *= -1;
                }
                return std::make_pair(std::array<Move, maxDepth>(), score);
            }

            constexpr enum Color Ocolor = Opposite(color);
            //Dynamic schedule since we do not know the ordering of moves or the number of moves in each call
            #pragma omp parallel for shared(alpha, beta) schedule(dynamic)
            for (uint8_t i = 0; i < moveList.Count(); i++) {
                // int thread = omp_get_thread_num();
                // // printf("%d\n", thread);
                // printf("I hit the for loop for thread %d \n", thread);
                if (alpha >= beta) {
                    continue; // Mimic cutoff because you cannot break in
                }
                //Private copy of the board for each thread
                StockDory::Board threadBoard = chessBoard;
                Move nextMove = moveList[i];
                Square from = nextMove.From();
                Square to = nextMove.To();
                Piece promotion = nextMove.Promotion();
                PreviousState prevState = threadBoard.Move<0>(from, to, promotion);
                std::pair<std::array<Move, maxDepth>, int> localResult = alphaBetaNegaParallel<Ocolor, maxDepth>(threadBoard, -beta, -alpha, depth - 1);
                localResult.second = -localResult.second;
                threadBoard.UndoMove<0>(prevState, from, to);
                #pragma omp critical
                {
                    if (localResult.second > bestScore) {
                        bestScore = localResult.second;
                        bestLine[0] = nextMove;
                        for (int j = 0; j < depth - 1; j++) {
                            bestLine[j + 1] = localResult.first[j];
                        }
                        alpha = std::max(alpha, bestScore);
                    }
                }
            }

            return std::make_pair(bestLine, bestScore);
        }

};

#endif //ENGINE_H
