//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>

#include "Information.h"

#include "UCI/UCIInterface.h"
#include "BenchHash.h"
#include "NetworkConverter.h"

void DisplayTitle()
{
    std::stringstream ss;
    ss << NAME << " " << VERSION << " - Neural Network: " << StockDory::Evaluation::Name() << "\n";
    ss << "Provided by " << AUTHOR << " under the " << LICENSE << " license.";

    std::cerr << ss.str() << std::endl;
}

int main(int argc, char* argv[])
{
    DisplayTitle();

    if (argc > 1) {
        if        (strutil::compare_ignore_case(argv[1], "bench")) {
            StockDory::BenchHash::Run();
            return EXIT_SUCCESS;
        } else if (strutil::compare_ignore_case(argv[1], "convert")) {
            StockDory::NetworkConverter::Launch();
            return EXIT_SUCCESS;
        }
    }

    if (argc > 1 && strutil::compare_ignore_case(argv[1], "bench")) {
        StockDory::BenchHash::Run();
        return EXIT_SUCCESS;
    }

    StockDory::UCIInterface::Launch();

    return EXIT_SUCCESS;
}
