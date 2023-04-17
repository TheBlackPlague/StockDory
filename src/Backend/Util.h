//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UTIL_H
#define STOCKDORY_UTIL_H

#include <sstream>
#include <vector>

#include "Type/Square.h"

#include "../External/MantaRay/src/IO/BinaryFileStream.h"

namespace StockDory
{

    class Util
    {

        public:
            static std::vector<std::string> Split(const std::string &s, char d)
            {
                std::vector<std::string> result;

                std::stringstream stream (s);

                std::string item;
                while(std::getline(stream, item, d)) {
                    result.push_back(item);
                }

                return result;
            }

            constexpr static Square StringToSquare(const std::string &s)
            {
                int file = tolower(s[0]) - 97;
                int rank = tolower(s[0]) - 49;

                return static_cast<Square>(rank * 8 + file);
            }
    };

} // StockDory

#endif //STOCKDORY_UTIL_H
