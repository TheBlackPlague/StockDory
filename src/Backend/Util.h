//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UTIL_H
#define STOCKDORY_UTIL_H

#include <sstream>
#include <vector>

#include "Type/Square.h"

namespace StockDory
{

    class Util
    {

        public:
            constexpr static Square StringToSquare(const std::string& s)
            {
                int file = tolower(s[0]) - 97;
                int rank = tolower(s[0]) - 49;

                return static_cast<Square>(rank * 8 + file);
            }

            static inline std::string SquareToString(const Square sq)
            {
                std::string s;
                s += static_cast<char>(tolower(File(sq)));
                s += Rank(sq);

                return s;
            }

    };

} // StockDory

#endif //STOCKDORY_UTIL_H
