//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UTIL_H
#define STOCKDORY_UTIL_H

#include <sstream>
#include <vector>
#include <iomanip>

#include "Type/Square.h"

namespace StockDory
{

    class Util
    {

        public:
            constexpr static inline Square StringToSquare(const std::string& s)
            {
                int file = tolower(s[0]) - 97;
                int rank = tolower(s[1]) - 49;

                return static_cast<Square>(rank * 8 + file);
            }

            static inline std::string SquareToString(const Square sq)
            {
                std::string s;
                s += static_cast<char>(tolower(File(sq)));
                s += Rank(sq);

                return s;
            }

            static inline std::string ToHex(const uint64_t value)
            {
                std::stringstream stream;
                stream << std::setfill('0') << std::setw(sizeof(uint64_t) * 2);
                stream << std::uppercase << std::hex << value;
                return stream.str();
            }

    };

} // StockDory

#endif //STOCKDORY_UTIL_H
