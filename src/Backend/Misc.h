//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_MISC_H
#define STOCKDORY_MISC_H

#include <iomanip>
#include <sstream>
#include <string>

template<typename T, typename = std::enable_if_t<std::is_fundamental_v<T>>>
std::string ToHex(const T v)
{
    std::stringstream ss;

    ss << std::setfill('0') << std::setw(sizeof(T) * 2);
    ss << std::uppercase << std::hex << v;

    return ss.str();
}

#endif //STOCKDORY_MISC_H
