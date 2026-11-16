//
// Copyright (c) 2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_OUTPUT_H
#define STOCKDORY_OUTPUT_H

#include <iostream>
#include <mutex>
#include <string_view>

namespace StockDory
{

    void Print(const std::string_view line)
    {
        static std::mutex mutex;

        const std::lock_guard lock (mutex);

        std::cout << line << std::endl;
    }

    void Error(const std::string_view message)
    {
        std::cerr << "ERROR: " << message << std::endl;
    }

} // StockDory

#endif //STOCKDORY_OUTPUT_H
