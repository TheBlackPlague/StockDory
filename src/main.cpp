//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <strutil.h>

#include "Information.h"

#include "Terminal/BenchHash.h"
#include "Terminal/UCI/UCI.h"

static void DisplayTitle()
{
    std::stringstream ss;

    ss << NAME << " " << VERSION << " (" << CODENAME << ")" << "\n";
    ss << "Copyright (c) " << COPYRIGHT << " " << AUTHOR << " - Licensed under the " << LICENSE;

    std::cerr << ss.str() << std::endl;
}

int main(const int argc, const char* argv[])
{
    DisplayTitle();

    if (argc > 1 && strutil::compare_ignore_case(argv[1], "bench")) {
        StockDory::BenchHash::Run();
    } else {
        StockDory::UCI().Launch();
    }

    StockDory::Engine::Shutdown();

    return EXIT_SUCCESS;
}
