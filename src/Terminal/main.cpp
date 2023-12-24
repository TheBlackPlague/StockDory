//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>
#include <cstdlib>

#include "Information.h"

#include "UCI/UCIInterface.h"
#include "BenchHash.h"
#include "NetworkConverter.h"

#include "../Statistical/UniquePositionCounter.h"

void DisplayTitle()
{
    std::stringstream ss;
    ss << NAME << " " << VERSION << " - Neural Architecture: " << StockDory::Evaluation::Name() << "\n";
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
        } else if (strutil::compare_ignore_case(argv[1], "upc")) {
            StockDory::UpcRunner::SetBoard(argv[2]);
            StockDory::UpcRunner::UniquePositionCount<true>(strutil::parse_string<uint32_t>(argv[3]));
            return EXIT_SUCCESS;
        }
    }

    StockDory::UCIInterface::Launch();

    return EXIT_SUCCESS;
}
