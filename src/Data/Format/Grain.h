//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_GRAIN_H
#define STOCKDORY_GRAIN_H

#include "../Type/Seed.h"

namespace StockDory
{

    template<GranaVersion Version>
    class Grain
    {

        using Seed  = Seed <Version>;
        using Stalk = Stalk<Version>;

        using StalkArray = std::array<Stalk, 63>;

        Seed       Internal0;
        StalkArray Internal1;

        public:
        Grain()
        {
            Internal0 = Seed();
            Internal1 =     {};
        }

        Grain(const Seed&& seed)
        {
            Internal0 = std::move(seed);
            Internal1 =              {};
        }

        Grain(std::istream& stream)
        {
            stream.read(reinterpret_cast<char*>(&Internal0), sizeof Internal0);

            const size_t remaining = Internal0.RemainingStalkCount() * sizeof(Stalk);

            stream.read(reinterpret_cast<char*>(&Internal1), remaining);
        }

        inline void Branch(const Stalk stalk)
        {
            Internal1[Internal0.PostIncrementRemainingStalkCount()] = stalk;
        }

        inline void Write(std::ostream& stream) const
        {
            stream.write(reinterpret_cast<const char*>(&Internal0), sizeof Internal0);

            const size_t remaining = Internal0.RemainingStalkCount() * sizeof(Stalk);

            stream.write(reinterpret_cast<const char*>(&Internal1), remaining);
        }

    };

} // StockDory

#endif //STOCKDORY_GRAIN_H
