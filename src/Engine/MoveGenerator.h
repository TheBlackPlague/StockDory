//
// Copyright (c) 2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_MOVEGENERATOR_H
#define STOCKDORY_MOVEGENERATOR_H

#include <cassert>

#include "Board.h"

namespace StockDory
{

    template<Side S>
    constexpr BitBoard LastRank = RayTable::Rank[RelativeRank<S>(7)];

    struct PawnTargets
    {

        BitBoard West          = BBDefault;
        BitBoard East          = BBDefault;
        BitBoard Single        = BBDefault;
        BitBoard Double        = BBDefault;
        BitBoard EnPassantFrom = BBDefault;

        Square EnPassantTo = NASQ;

    };

    template<Side Us, MoveSet Set>
    constexpr BitBoard PushMask()
    {
        return (Set & QuietMoves     ? ~LastRank<Us> : BBDefault) |
               (Set & PromotionMoves ?  LastRank<Us> : BBDefault) ;
    }

    template<Side Us>
    [[clang::always_inline]]
    void EnPassantCaptures(const Board& board, const BitBoard origin, const CheckBitBoard& check, PawnTargets& targets)
    {
        constexpr Side Them = Opposite(Us);

        const Square enPassantSquare = board.EnPassantSquare();

        if (enPassantSquare == NASQ) return;

        const Square enPassantPawnSquare = enPassantSquare - Up<Us>;

        assert(Get(board.PieceBoard<Them>(Pawn), enPassantPawnSquare) && !Get(board.Occupied(), enPassantSquare));

        if (!(check.Check & (FromSquare(enPassantSquare) | FromSquare(enPassantPawnSquare)))) return;

        const BitBoard capturers = AttackTable::Pawn[Them][enPassantSquare] & board.PieceBoard<Us>(Pawn) & origin;

        if (!capturers) return;

        const Square   king     = ToSquare(board.PieceBoard< Us >(King  ));
        const BitBoard queen    =          board.PieceBoard<Them>(Queen ) ;
        const BitBoard straight = queen |  board.PieceBoard<Them>(Rook  ) ;
        const BitBoard diagonal = queen |  board.PieceBoard<Them>(Bishop) ;

        const BitBoard occupied = (board.Occupied() & ~FromSquare(enPassantPawnSquare)) | FromSquare(enPassantSquare);

        for (const Square from : Squares(capturers)) {
            const BitBoard after = occupied & ~FromSquare(from);

            if (!(AttackTable::Rook  (king, after) & straight) &&
                !(AttackTable::Bishop(king, after) & diagonal))
                targets.EnPassantFrom |= FromSquare(from);
        }

        targets.EnPassantTo = enPassantSquare;
    }

    template<Side S>
    [[clang::always_inline]]
    BitBoard AttackMap(const Board& board, const BitBoard occupied)
    {
        const BitBoard pawns    =         board.PieceBoard<S>(Pawn  );
        const BitBoard queen    =         board.PieceBoard<S>(Queen );
        const BitBoard diagonal = queen | board.PieceBoard<S>(Bishop);
        const BitBoard straight = queen | board.PieceBoard<S>(Rook  );

        BitBoard attacked = Shift<UpWest<S>>(pawns) | Shift<UpEast<S>>(pawns);

        for (const Square sq : Squares(board.PieceBoard<S>(Knight))) attacked |= AttackTable::Knight[sq];

        for (const Square sq : Squares(diagonal)) attacked |= AttackTable::Bishop(sq, occupied);
        for (const Square sq : Squares(straight)) attacked |= AttackTable::Rook  (sq, occupied);

        return attacked | AttackTable::King[ToSquare(board.PieceBoard<S>(King))];
    }

    template<Side Us>
    [[clang::always_inline]]
    bool IsAttacked(const Board& board, const Square sq, const BitBoard occupied)
    {
        constexpr Side Them = Opposite(Us);

        if (AttackTable::Pawn[Us][sq] & board.PieceBoard<Them>(Pawn  )) return true;
        if (AttackTable::Knight  [sq] & board.PieceBoard<Them>(Knight)) return true;
        if (AttackTable::King    [sq] & board.PieceBoard<Them>(King  )) return true;

        const BitBoard queen = board.PieceBoard<Them>(Queen);

        if (AttackTable::Bishop(sq, occupied) & (queen | board.PieceBoard<Them>(Bishop))) return true;

        return AttackTable::Rook(sq, occupied) & (queen | board.PieceBoard<Them>(Rook));
    }

    template<Side Us, MoveSet Set, typename Sink>
    [[clang::always_inline]]
    void GenerateFrom(const Board& board, const GenerationMasks& masks, const BitBoard origin, Sink& sink)
    {
        constexpr Side Them = Opposite(Us);

        const BitBoard us    = board[ Us ];
        const BitBoard them  = board[Them];
        const BitBoard occ   = us | them;
        const BitBoard empty = ~occ;

        const CheckBitBoard& check = masks.Check;
        const PinBitBoard&   pin   = masks.Pin  ;

        if (!check.DoubleCheck) {
            const BitBoard quiet   = Set & QuietMoves   ? empty & check.Check : BBDefault;
            const BitBoard capture = Set & CaptureMoves ? them  & check.Check : BBDefault;

            const BitBoard pawns    = board.PieceBoard<Us>(Pawn) & origin;
            const BitBoard capturer = pawns & ~pin.Straight;
            const BitBoard pusher   = pawns & ~pin.Diagonal;

            const BitBoard step = (Shift<Up<Us>>(pusher & ~pin.Straight)               |
                                   Shift<Up<Us>>(pusher &  pin.Straight) & pin.Straight ) & empty;

            PawnTargets targets;

            targets.West = (Shift<UpWest<Us>>(capturer & ~pin.Diagonal)               |
                            Shift<UpWest<Us>>(capturer &  pin.Diagonal) & pin.Diagonal) & capture;
            targets.East = (Shift<UpEast<Us>>(capturer & ~pin.Diagonal)               |
                            Shift<UpEast<Us>>(capturer &  pin.Diagonal) & pin.Diagonal) & capture;

            targets.Single =               step                                        & check.Check;
            targets.Double = Shift<Up<Us>>(step & RayTable::Rank[RelativeRank<Us>(2)]) & check.Check & empty;

            targets.Single &= PushMask<Us, Set>();
            targets.Double &= PushMask<Us, Set>();

            if (Set & CaptureMoves) EnPassantCaptures<Us>(board, origin, check, targets);

            sink.PawnMoves(targets);

            const BitBoard knights = board.PieceBoard<Us>(Knight) & origin & ~(pin.Straight | pin.Diagonal);

            for (const Square sq : Squares(knights)) {
                const BitBoard attack = AttackTable::Knight[sq];

                sink.template PieceMoves<Knight>(sq, attack & quiet, attack & capture);
            }

            for (const Square sq : Squares(board.PieceBoard<Us>(Bishop) & origin & ~pin.Straight)) {
                BitBoard attack = AttackTable::Bishop(sq, occ);

                if (Get(pin.Diagonal, sq)) attack &= pin.Diagonal;

                sink.template PieceMoves<Bishop>(sq, attack & quiet, attack & capture);
            }

            for (const Square sq : Squares(board.PieceBoard<Us>(Rook) & origin & ~pin.Diagonal)) {
                BitBoard attack = AttackTable::Rook  (sq, occ);

                if (Get(pin.Straight, sq)) attack &= pin.Straight;

                sink.template PieceMoves<Rook>(sq, attack & quiet, attack & capture);
            }

            for (const Square sq : Squares(board.PieceBoard<Us>(Queen) & origin)) {
                BitBoard attack = BBDefault;

                if (!Get(pin.Diagonal, sq))
                    attack |= AttackTable::Rook  (sq, occ) & (Get(pin.Straight, sq) ? pin.Straight : BBFilled);

                if (!Get(pin.Straight, sq))
                    attack |= AttackTable::Bishop(sq, occ) & (Get(pin.Diagonal, sq) ? pin.Diagonal : BBFilled);

                sink.template PieceMoves<Queen>(sq, attack & quiet, attack & capture);
            }
        }

        const BitBoard king = board.PieceBoard<Us>(King) & origin;

        if (!king) return;

        const Square sq = ToSquare(king);

        const BitBoard candidates = AttackTable::King[sq] & ((Set & QuietMoves   ? empty : BBDefault) |
                                                             (Set & CaptureMoves ? them  : BBDefault) );

        const bool mayCastle = Set & QuietMoves && check.Check == BBFilled &&
                               (board.CastlingRight<Us>(KingSide) || board.CastlingRight<Us>(QueenSide));

        if (!candidates && !mayCastle) return;

        const BitBoard occupied = occ ^ king;

        const BitBoard queen    =         board.PieceBoard<Them>(Queen);
        const BitBoard diagonal = queen | board.PieceBoard<Them>(Bishop);
        const BitBoard straight = queen | board.PieceBoard<Them>(Rook  );

        BitBoard steps   = BBDefault;
        BitBoard castles = BBDefault;

        if (!mayCastle && 2 * Count(candidates) < Count(diagonal) + Count(straight)) {
            for (const Square to : Squares(candidates))
                if (!IsAttacked<Us>(board, to, occupied)) steps |= FromSquare(to);
        } else {
            const BitBoard attacked = AttackMap<Them>(board, occupied);

            steps = candidates & ~attacked;

            if (mayCastle) {
                const Castling& castling = board.GetCastling();

                for (const CastlingSide castle : { KingSide, QueenSide }) {
                    if (!board.CastlingRight<Us>(castle)) continue;

                    if (occ      & castling.    Path[Us][castle]) continue;
                    if (attacked & castling.KingPath[Us][castle]) continue;

                    if (Get(pin.Straight, castling.RookFrom[Us][castle])) continue;

                    castles |= FromSquare(castling.RookFrom[Us][castle]);
                }
            }
        }

        sink.KingMoves(sq, steps & empty, steps & them, castles);
    }

    template<Side Us, MoveSet Set, typename Sink>
    [[clang::always_inline]]
    void Generate(const Board& board, Sink& sink)
    {
        GenerateFrom<Us, Set>(board, board.Masks<Us>(), BBFilled, sink);
    }

    struct MoveCounter
    {

        uint8_t Count = 0;

        [[clang::always_inline]]
        void PawnMoves(const PawnTargets& targets)
        {
            constexpr BitBoard PromotionRanks = RayTable::Rank[0] | RayTable::Rank[7];

            Count += StockDory::Count(targets.West  ) + StockDory::Count(targets.East         ) +
                     StockDory::Count(targets.Single) + StockDory::Count(targets.Double       ) +
                                                        StockDory::Count(targets.EnPassantFrom) ;

            if ((targets.West | targets.East | targets.Single) & PromotionRanks)
                Count += 3 * (StockDory::Count(targets.West    & PromotionRanks) +
                              StockDory::Count(targets.East    & PromotionRanks) +
                              StockDory::Count(targets.Single  & PromotionRanks) );
        }

        template<PieceType Type>
        [[clang::always_inline]]
        void PieceMoves(const Square _, const BitBoard quiets, const BitBoard captures)
        {
            Count += StockDory::Count(quiets | captures);
        }

        [[clang::always_inline]]
        void KingMoves(const Square _, const BitBoard quiets, const BitBoard captures, const BitBoard castles)
        {
            Count += StockDory::Count(quiets | captures | castles);
        }

    };

    template<Side Us>
    uint8_t MoveCount(const Board& board)
    {
        MoveCounter counter;

        Generate<Us, AllMoves>(board, counter);

        return counter.Count;
    }

    uint8_t MoveCount(const Board& board)
    {
        return board.SideToMove() == White ? MoveCount<White>(board) : MoveCount<Black>(board);
    }

} // StockDory

#endif //STOCKDORY_MOVEGENERATOR_H
