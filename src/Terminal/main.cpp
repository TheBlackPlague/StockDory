//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>

#include "Information.h"

#include "BenchHash.h"
#include "NetworkConverter.h"
#include "UCI/UCIInterface.h"

void DisplayTitle()
{
    std::stringstream ss;
    ss << NAME << " " << VERSION << " - Neural Architecture: " << StockDory::Evaluation::Name() << "\n";
    ss << "Provided by " << AUTHOR << " under the " << LICENSE << " license.";

    std::cerr << ss.str() << std::endl;
}

int main(const int argc, const char* argv[])
{
    DisplayTitle();

    if (argc > 1) {
        if (strutil::compare_ignore_case(argv[1], "bench"  )) {
            StockDory::BenchHash::Run();
            return EXIT_SUCCESS;
        }

        if (strutil::compare_ignore_case(argv[1], "convert")) {
            StockDory::NetworkConverter::Launch();
            return EXIT_SUCCESS;
        }
    }

    StockDory::UCIInterface::Launch();

    return EXIT_SUCCESS;
}
