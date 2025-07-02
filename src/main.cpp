//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>

#include "Information.h"

#include "Terminal/BenchHash.h"
#include "Terminal/UCI/UCIInterface.h"

void DisplayTitle()
{
    std::stringstream ss;

    ss << NAME << " " << VERSION << " (" << CODENAME << ")" << "\n";
    ss << "Copyright (c) 2025 " << AUTHOR << " - Licensed under the " << LICENSE;

    std::cerr << ss.str() << std::endl;
}

int main(const int argc, const char* argv[])
{
    StockDory::Evaluation::Initialize();

    DisplayTitle();

    if (argc > 1) {
        if (strutil::compare_ignore_case(argv[1], "bench"  )) {
            StockDory::BenchHash::Run();
            return EXIT_SUCCESS;
        }
    }

    StockDory::UCIInterface::Launch();

    return EXIT_SUCCESS;
}
