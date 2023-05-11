//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>
#include <chrono>

#include "../Backend/Information.h"

#include "UCI/UCIInterface.h"

#include "../Engine/Search.h"

int main()
{
    std::cout << Title << " " << Version << std::endl;
    std::cout << "Provided by " << Author << " under the " << License << " license." << std::endl;

    StockDory::Board board;

    const uint8_t depth = 18;
    StockDory::Search search(board);

    auto start = std::chrono::high_resolution_clock::now();
    search.IterativeDeepening(depth);
    auto stop  = std::chrono::high_resolution_clock::now();

    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

    std::pair<int32_t, Move> result = search.Result();
    int32_t evaluation = result.first ;

    std::cout << "Evaluation: " << evaluation << std::endl;
    std::cout << "PV: " << search.PvLine() << std::endl;
    std::cout << "Searched " << search.SearchedNodes() << " nodes in ";
    std::cout << time << "ms." << std::endl;

    return 0;
}
