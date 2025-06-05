//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_WDL_H
#define STOCKDORY_WDL_H

namespace StockDory
{

    class WDLCalculator
    {

        constexpr static double Scale = 1000.0;

        struct Coefficient { double A = 0.0; double B = 0.0; };

        enum Weight : uint8_t { A, B };

        constexpr static Array<double, 2, 4> WValue = {{
            {  190.541, -461.949,  185.975,  334.376 },
            {  106.738, -285.539,  297.152, -  9.669 }
        }};

        template<Weight W>
        [[clang::always_inline]]
        static double Formula(const Score x)
        {
            return ((WValue[W][0] * x / 58 + WValue[W][1]) * x / 58 + WValue[W][2]) * x / 58 + WValue[W][3];
        }

        [[clang::always_inline]]
        static Coefficient Coefficient(const Board& board)
        {
            const BitBoard pawn   = board.PieceBoard(Pawn  , White) | board.PieceBoard(Pawn  , Black);
            const BitBoard knight = board.PieceBoard(Knight, White) | board.PieceBoard(Knight, Black);
            const BitBoard bishop = board.PieceBoard(Bishop, White) | board.PieceBoard(Bishop, Black);
            const BitBoard rook   = board.PieceBoard(Rook  , White) | board.PieceBoard(Rook  , Black);
            const BitBoard queen  = board.PieceBoard(Queen , White) | board.PieceBoard(Queen , Black);

            const Score mat = Count(pawn  ) * 1 +
                              Count(knight) * 3 +
                              Count(bishop) * 3 +
                              Count(rook  ) * 5 +
                              Count(queen ) * 9 ;

            return { Formula<A>(mat), Formula<B>(mat) };
        }

        public:
        [[clang::always_inline]]
        static Score W(const Board& board, const Score cp)
        {
            const auto [a, b] = Coefficient(board);

            return round(Scale / (1 + exp((a - cp) / b)));
        }

        [[clang::always_inline]]
        static Score L(const Board& board, const Score cp)
        {
            return W(board, -cp);
        }

        [[clang::always_inline]]
        static Score D(const Board& board, const Score cp) { return Scale - W(board, cp) - L(board, cp); }

        [[clang::always_inline]]
        static Score S(const Board& board, const Score cp)
        {
            if (cp == 0 || abs(cp) >= Mate - MaxDepth) return cp;

            const auto [a, b] = Coefficient(board);

            return round((Scale / 10) * cp / a);
        }

    };

    struct WDL
    {

        Score W; Score D; Score L;

        WDL() : W(0), D(1000), L(0) {}

        [[clang::always_inline]]
        WDL(const Board& board, const Score cp) : W(WDLCalculator::W(board, cp)),
                                                  D(WDLCalculator::D(board, cp)),
                                                  L(WDLCalculator::L(board, cp)) {}

    };

} // StockDory

#endif //STOCKDORY_WDL_H
