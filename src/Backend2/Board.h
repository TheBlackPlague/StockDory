//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_BOARD_H
#define STOCKDORY_BOARD_H

#include "Move.h"
#include "Position.h"

namespace StockDory
{

    struct NullMovePriorState { Square EnPassant = InvalidSquare; };

    struct MovePriorState
    {

    };

    template<class EvaluationHandler = DefaultEvaluationHandler>
    class Board : Position<EvaluationHandler>
    {

        public:
        void MakeNullMove()
        {
            NullMovePriorState state;

            if (this->Info.EnPassant != InvalidSquare) {
                state.EnPassant = this->Info.EnPassant;
                this->Zobrist = ZobristHash(this->Zobrist, this->Info.EnPassant);

                this->Info.EnPassant = InvalidSquare;
            }

            this->Info.FlipSideToMove();
            this->Zobrist = ZobristHash(this->Zobrist);
        }

        void UndoNullMove(const NullMovePriorState& state)
        {
            if (state.EnPassant != InvalidSquare) {
                this->Info.EnPassant = state.EnPassant;
                this->Zobrist = ZobristHash(this->Zobrist, this->Info.EnPassant);
            }

            this->Info.FlipSideToMove();
            this->Zobrist = ZobristHash(this->Zobrist);
        }

        template<Side Side>
        void MakeMove(const Move<> move)
        {
            assert(this->Info.SideToMove() == Side, "Provided side to move does not align with position info");

            const Square origin = move.Origin();
            const Square target = move.Target();

            const Piece originPiece = this->PieceArray[origin];
            const Piece targetPiece = this->PieceArray[target];

            const bool canCastle = Side == White ? this->Info.template Castling<KWhite | QWhite>() :
                                                   this->Info.template Castling<KBlack | QBlack>() ;
            if (canCastle) {
                switch (originPiece.Type()) {
                    case King:
                        this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());

                        (Side == White ? this->Info.template Castling<KWhite | QWhite, false>() :
                                         this->Info.template Castling<KBlack | QBlack, false>());

                        this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());
                        break;
                    case Rook:
                        if        (origin == (Side == White ? A1 : A8)) {
                            this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());

                            (Side == White ? this->Info.template Castling<QWhite, false>() :
                                             this->Info.template Castling<QBlack, false>());

                            this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());
                        } else if (origin == (Side == White ? H1 : H8)) {
                            this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());

                            (Side == White ? this->Info.template Castling<KWhite, false>() :
                                             this->Info.template Castling<KBlack, false>());

                            this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());
                        }
                        break;
                    default: break;
                }
            }

            if (!move.IsCapture()) {
                auto HandleCastling = [&]<CastlingDirection Direction>()
                {
                    this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());

                    (Side == White ? this->Info.template Castling<KWhite | QWhite, false>() :
                                     this->Info.template Castling<KBlack | QBlack, false>());

                    this->Zobrist = ZobristHash(this->Zobrist, this->Info.CastlingRaw());

                    constexpr Square RookOrigin = Direction == K ? (Side == White ? H1 : H8) :
                                                                   (Side == White ? A1 : A8) ;
                    constexpr Square RookTarget = Direction == K ? (Side == White ? F1 : F8) :
                                                                   (Side == White ? D1 : D8) ;

                    Set<false>(this->PieceBB[Side][King], origin);
                    Set<true >(this->PieceBB[Side][King], target);

                    Set<false>(this->PieceBB[Side][Rook], RookOrigin);
                    Set<true >(this->PieceBB[Side][Rook], RookTarget);

                    Set<false>(this->SideBB[Side],     origin);
                    Set<true >(this->SideBB[Side],     target);
                    Set<false>(this->SideBB[Side], RookOrigin);
                    Set<true >(this->SideBB[Side], RookTarget);

                    this->PieceArray[    origin] = { InvalidSide, InvalidPieceType };
                    this->PieceArray[RookOrigin] = { InvalidSide, InvalidPieceType };

                    this->PieceArray[    target] = { Side, King };
                    this->PieceArray[RookTarget] = { Side, Rook };

                    this->Zobrist = ZobristHash(this->Zobrist, { Side, King }, origin);
                    this->Zobrist = ZobristHash(this->Zobrist, { Side, King }, target);

                    this->Zobrist = ZobristHash(this->Zobrist, { Side, Rook }, RookOrigin);
                    this->Zobrist = ZobristHash(this->Zobrist, { Side, Rook }, RookTarget);
                };

                if (move.IsCastling<K>()) {
                    HandleCastling.template operator()<K>();
                    return;
                }

                if (move.IsCastling<Q>()) {
                    HandleCastling.template operator()<Q>();
                    return;
                }

                if (move.IsPromotion()) {
                    // TODO: Handle Hashing
                    PieceType type = move.PromotionType();

                    Set<false>(this->PieceBB[Side][Pawn], origin);
                    Set<true >(this->PieceBB[Side][type], target);

                    Set<false>(this->SideBB[Side], origin);
                    Set<true >(this->SideBB[Side], target);

                    this->PieceArray[origin] = { InvalidSide, InvalidPieceType };
                    this->PieceArray[target] = {        Side,             type };

                    return;
                }

                if (originPiece.Type() == Pawn) {
                    if (static_cast<Square>(origin ^ 16) == target) {
                        const auto ep = static_cast<Square>(target ^ 8);

                        if (Attack::Pawn[Side][ep] & this->PieceBB[~Side][Pawn]) {
                            this->Property.SetEnPassantSquare(ep);
                            // TODO: Handle Hashing
                        }
                    }
                }
            } else {
                if (this->Property.template CanCastle<~Side>() && targetPiece.Type() == Rook) {
                    // TODO: Handle Hashing
                    if      (target == (Side == White ? A8 : A1))
                        this->Property.template InvalidateCastlingRights<~Side, Q>();
                    else if (target == (Side == White ? H8 : H1))
                        this->Property.template InvalidateCastlingRights<~Side, K>();
                }


            }
        }

        private:
        void Remove(const Side side, const PieceType type, const Square sq)
        {
            Set<false>(this->PieceBB[side][type], sq);
            Set<false>(this-> SideBB[side]      , sq);

            this->PieceArray[sq] = { InvalidSide, InvalidPieceType };
        }

        void Insert(const Side side, const PieceType type, const Square sq)
        {
            Set<true>(this->PieceBB[side][type], sq);
            Set<true>(this-> SideBB[side]      , sq);

            this->PieceArray[sq] = { side, type };
        }

    };

} // StockDory

#endif //STOCKDORY_BOARD_H
