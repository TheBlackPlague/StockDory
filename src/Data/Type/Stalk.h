//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_STALK_H
#define STOCKDORY_STALK_H

#include "../../Backend/Type/Move.h"

#include "../Format/GranaVersion.h"

namespace StockDory
{

    using ScoreType = int16_t;

    template<GranaVersion Version>
    struct Stalk
    {

        private:
        Move      Internal0;
        ScoreType Internal1;

        public:
        constexpr Stalk()
        {
            Internal0 = Move();
            Internal1 =      0;
        }

        constexpr Stalk(const Move move, const ScoreType score)
        {
            Internal0 =  move;
            Internal1 = score;
        }

        constexpr Move Move() const
        {
            return Internal0;
        }

        constexpr ScoreType Score() const
        {
            return Internal1;
        }

    };

} // StockDory

#endif //STOCKDORY_STALK_H
