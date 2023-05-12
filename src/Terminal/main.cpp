//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>

#include "../Backend/Information.h"

#include "UCI/UCIInterface.h"
#include "BenchHash.h"

int main(int argc, char* argv[])
{
    std::cout << Title << " " << Version << std::endl;
    std::cout << "Provided by " << Author << " under the " << License << " license." << std::endl;

    if (argc > 1 && strutil::compare_ignore_case(argv[1], "bench")) {
        StockDory::BenchHash::Run();
        return EXIT_SUCCESS;
    }

    StockDory::UCIInterface::Launch();

    return EXIT_SUCCESS;
}
