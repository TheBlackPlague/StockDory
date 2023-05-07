//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>

#include "../Backend/Information.h"

#include "UCI/UCIInterface.h"

#include "../Engine/Search.h"

int main()
{
    std::cout << Title << " " << Version << std::endl;
    std::cout << "Provided by " << Author << " under the " << License << " license." << std::endl;

    StockDory::Board board;

    StockDory::Search search(board);

    Move best = search.IterativeDeepening(1);

    std::cout << StockDory::Util::SquareToString(best.From()) << StockDory::Util::SquareToString(best.To()) << std::endl;

//    StockDory::UCIInterface::Launch();

    return 0;
}
