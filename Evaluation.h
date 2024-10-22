//
// Created by Capks on 10/22/2024.
//

#ifndef EVALUATION_H
#define EVALUATION_H
#include "Backend/Board.h"
#include<cstdlib>

class Evaluation {
public:
    //Create evaluation function
    float eval(StockDory::Board board) {
        int random = rand();
        auto result = (float)(-5 + random % (11));
        return result;
    }
};

#endif //EVALUATION_H
