//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_BOARD_H
#define STOCKDORY_BOARD_H

#include <array>
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Misc.h"
#include "Type/BitBoard.h"
#include "Type/CheckBitBoard.h"
#include "Type/Color.h"
#include "Type/Move.h"
#include "Type/Piece.h"
#include "Type/PieceColor.h"
#include "Type/PinBitBoard.h"
#include "Type/PreviousState.h"
#include "Type/Zobrist.h"

#include "Template/MoveType.h"
#include "Move/AttackTable.h"
#include "Move/BlackMagicFactory.h"
#include "Move/RayTable.h"
#include "../Engine/Evaluation.h"

namespace StockDory
{

    enum class BoardType : uint8_t
    {

        Engine,
        Perft,
        Packed

    };

    struct EmptyBoardState {};

    template<BoardType Profile>
    class BasicBoard
    {

        static_assert(Profile != BoardType::Packed);

        template<BoardType> friend class BasicBoard;

        constexpr static bool Engine = Profile == BoardType::Engine;

        constexpr static uint8_t CastlingMask     = 0xF;
        constexpr static uint8_t WhiteKCastleMask = 0x8;
        constexpr static uint8_t WhiteQCastleMask = 0x4;
        constexpr static uint8_t BlackKCastleMask = 0x2;
        constexpr static uint8_t BlackQCastleMask = 0x1;
        constexpr static uint8_t ColorFlipMask    = 0x10;

        constexpr static Array<uint8_t, 2> ColorCastleMask {
            WhiteKCastleMask | WhiteQCastleMask,
            BlackKCastleMask | BlackQCastleMask
        };

        constexpr static Array<uint8_t, 64> CastleMask = []
        {
            Array<uint8_t, 64> result {};
            result.fill(CastlingMask | ColorFlipMask);
            result[A1] &= ~WhiteQCastleMask;
            result[H1] &= ~WhiteKCastleMask;
            result[E1] &= ~(WhiteKCastleMask | WhiteQCastleMask);
            result[A8] &= ~BlackQCastleMask;
            result[H8] &= ~BlackKCastleMask;
            result[E8] &= ~(BlackKCastleMask | BlackQCastleMask);
            return result;
        }();

        Array<BitBoard, Engine ? 2 : 1, 6> BB {};
        Array<PieceColor, 64> PieceAndColor {};
        Array<BitBoard, Engine ? 3 : 2> ColorBB {};

        uint8_t CastlingRightAndColorToMove = 0;
        std::conditional_t<Engine, BitBoard, Square> EnPassantTarget {};
        [[no_unique_address]] std::conditional_t<Engine, ZobristHash, EmptyBoardState> Hash {};

        public:
        BasicBoard() : BasicBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}

        explicit BasicBoard(const std::string_view fen)
        {
            Fill(PieceAndColor, PieceColor(NAP, NAC));
            SetEnPassant(NASQ);

            Array<std::string_view, 6> fields {};
            size_t offset = 0;
            for (auto& field : fields) {
                offset = fen.find_first_not_of(' ', offset);
                if (offset == std::string_view::npos) throw std::invalid_argument("Incomplete FEN");
                const size_t end = fen.find(' ', offset);
                field = fen.substr(offset, end == std::string_view::npos ? end : end - offset);
                offset = end;
            }
            if (offset != std::string_view::npos && fen.find_first_not_of(' ', offset) != std::string_view::npos)
                throw std::invalid_argument("Excess FEN fields");

            int rank = 7;
            int file = 0;
            for (const char value : fields[0]) {
                if (value == '/') {
                    if (file != 8 || rank == 0) throw std::invalid_argument("Invalid FEN rank");
                    rank--;
                    file = 0;
                    continue;
                }
                if (value >= '1' && value <= '8') {
                    file += value - '0';
                    if (file > 8) throw std::invalid_argument("Invalid FEN rank");
                    continue;
                }
                if (file >= 8) throw std::invalid_argument("Invalid FEN square");

                const size_t index = std::string_view("PNBRQKpnbrqk").find(value);
                if (index == std::string_view::npos) throw std::invalid_argument("Invalid FEN piece");
                const auto piece = static_cast<Piece>(index % 6);
                const Color color = index < 6 ? White : Black;
                InsertNative(piece, color, static_cast<Square>(rank * 8 + file++));
            }
            if (rank != 0 || file != 8) throw std::invalid_argument("Invalid FEN placement");
            if (fields[1] != "w" && fields[1] != "b") throw std::invalid_argument("Invalid FEN color");
            CastlingRightAndColorToMove = fields[1] == "w" ? 0 : ColorFlipMask;

            for (const char right : fields[2]) {
                switch (right) {
                    case 'K': CastlingRightAndColorToMove |= WhiteKCastleMask; break;
                    case 'Q': CastlingRightAndColorToMove |= WhiteQCastleMask; break;
                    case 'k': CastlingRightAndColorToMove |= BlackKCastleMask; break;
                    case 'q': CastlingRightAndColorToMove |= BlackQCastleMask; break;
                    case '-': break;
                    default: throw std::invalid_argument("Invalid FEN castling rights");
                }
            }

            if (fields[3] != "-") {
                if (fields[3].size() != 2 || fields[3][0] < 'a' || fields[3][0] > 'h' ||
                    (fields[3][1] != '3' && fields[3][1] != '6'))
                    throw std::invalid_argument("Invalid FEN en passant square");
                const auto ep = static_cast<Square>((fields[3][1] - '1') * 8 + fields[3][0] - 'a');
                if (AttackTable::Pawn[Opposite(ColorToMove())][ep] & PieceBoard(Pawn, ColorToMove()))
                    SetEnPassant(ep);
            }

            UpdateNACBB();
            if constexpr (Engine) Hash = ComputeHash();
        }

        template<BoardType Other> requires (Other != Profile)
        explicit BasicBoard(const BasicBoard<Other>& other)
        {
            if constexpr (Other != BoardType::Packed) {
                PieceAndColor = other.PieceAndColor;
                ColorBB[White] = other.ColorBB[White];
                ColorBB[Black] = other.ColorBB[Black];
                for (Piece piece = Pawn; piece != NAP; piece = Next(piece)) {
                    if constexpr (Engine) {
                        BB[White][piece] = other.PieceBoard(piece, White);
                        BB[Black][piece] = other.PieceBoard(piece, Black);
                    } else BB[0][piece] = other.PieceBoard(piece, White) | other.PieceBoard(piece, Black);
                }
            } else {
                Fill(PieceAndColor, PieceColor(NAP, NAC));
                BitBoardIterator iterator(~other[NAC]);
                for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                    const PieceColor pc = other[sq];
                    InsertNative(pc.Piece(), pc.Color(), sq);
                }
            }
            CastlingRightAndColorToMove = other.CastlingRights() | other.ColorToMove() << 4;
            SetEnPassant(other.EnPassantSquare());
            UpdateNACBB();
            if constexpr (Engine) {
                if constexpr (Other == BoardType::Packed) Hash = ComputeHash();
                else Hash = other.Zobrist();
            }
        }

        void LoadForEvaluation(const size_t threadId = 0) const
        {
            Evaluation::ResetNetworkState(threadId);
            BitBoardIterator iterator(~Empty());
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const PieceColor pc = PieceAndColor[sq];
                Evaluation::Activate(pc.Piece(), pc.Color(), sq, threadId);
            }
        }

        [[nodiscard]]
        std::string Fen() const
        {
            std::string result;
            result.reserve(90);
            for (int rank = 7; rank >= 0; rank--) {
                int empty = 0;
                for (int file = 0; file < 8; file++) {
                    const PieceColor pc = PieceAndColor[rank * 8 + file];
                    if (pc.Piece() == NAP) {
                        empty++;
                        continue;
                    }
                    if (empty) result += static_cast<char>('0' + std::exchange(empty, 0));
                    result += std::string_view("PNBRQKpnbrqk")[pc.Piece() + pc.Color() * 6];
                }
                if (empty) result += static_cast<char>('0' + empty);
                if (rank) result += '/';
            }
            result += ColorToMove() == White ? " w " : " b ";
            if (CastlingRights()) {
                if (CastlingRights() & WhiteKCastleMask) result += 'K';
                if (CastlingRights() & WhiteQCastleMask) result += 'Q';
                if (CastlingRights() & BlackKCastleMask) result += 'k';
                if (CastlingRights() & BlackQCastleMask) result += 'q';
            } else result += '-';
            result += ' ';
            result += EnPassantSquare() == NASQ ? "-" : ::ToString(EnPassantSquare());
            result += " 0 1";
            return result;
        }

        [[nodiscard]]
        ZobristHash Zobrist() const
        {
            if constexpr (Engine) return Hash;
            else return ComputeHash();
        }

        [[nodiscard]]
        PieceColor operator [](const Square sq) const { return PieceAndColor[sq]; }

        [[nodiscard]]
        BitBoard operator [](const Color color) const { return color == NAC ? Empty() : ColorBB[color]; }

        template<Color Color>
        [[nodiscard]]
        BitBoard PieceBoard(const Piece piece) const { return PieceBoard(piece, Color); }

        [[nodiscard]]
        BitBoard PieceBoard(const Piece piece, const Color color) const
        {
            assert(piece != NAP && color != NAC);
            if constexpr (Engine) return BB[color][piece];
            else return BB[0][piece] & ColorBB[color];
        }

        [[nodiscard]]
        Color ColorToMove() const { return static_cast<Color>(CastlingRightAndColorToMove >> 4); }

        [[nodiscard]]
        uint8_t CastlingRights() const { return CastlingRightAndColorToMove & CastlingMask; }

        template<Color Color>
        [[nodiscard]]
        bool CastlingRightK() const
        {
            static_assert(Color != NAC);
            return CastlingRights() & (Color == White ? WhiteKCastleMask : BlackKCastleMask);
        }

        template<Color Color>
        [[nodiscard]]
        bool CastlingRightQ() const
        {
            static_assert(Color != NAC);
            return CastlingRights() & (Color == White ? WhiteQCastleMask : BlackQCastleMask);
        }

        [[nodiscard]]
        BitBoard EnPassant() const
        {
            if constexpr (Engine) return EnPassantTarget;
            else return EnPassantTarget == NASQ ? BBDefault : FromSquare(EnPassantTarget);
        }

        [[nodiscard]]
        Square EnPassantSquare() const
        {
            if constexpr (Engine) return ToSquare(EnPassantTarget);
            else return EnPassantTarget;
        }

        template<Color We>
        bool Checked() const
        {
            constexpr Color by = Opposite(We);

            const Square king = ToSquare(PieceBoard(King, We));

            if (AttackTable::Pawn[We][king] & PieceBoard(Pawn, by)) return true;

            if (AttackTable::Knight[king] & PieceBoard(Knight, by)) return true;

            const BitBoard occupied = ~Empty();
            const BitBoard queen    = PieceBoard(Queen, by);

            if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, king, occupied)] &
                (queen | PieceBoard(Bishop, by)))
                return true;

            if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook  , king, occupied)] &
                (queen | PieceBoard( Rook , by)))
                return true;

            return AttackTable::King[king] & PieceBoard(King, by);
        }

        template<Color By>
        CheckBitBoard Check() const
        {
            uint8_t count = 0;
            auto    check = CheckBitBoard();

            const Square sq = ToSquare(PieceBoard(King, Opposite(By)));

            // Check if the square is under attack by opponent knights or pawns.
            const BitBoard pawnCheck   = AttackTable::Pawn[Opposite(By)][sq] & PieceBoard(Pawn, By);
            const BitBoard knightCheck = AttackTable::Knight[sq] & PieceBoard(Knight, By);

            // If the square is under attack by a pawn or knight, add it our checks.
            check.Check |= pawnCheck;
            check.Check |= knightCheck;

            // Increment the count if there are checks.
            count += static_cast<bool>(pawnCheck);
            count += static_cast<bool>(knightCheck);

            // Check if the square is under attack by opponent bishops, rooks, or queens.
            // For queen, we can merge with checks for bishop and rook.
            const BitBoard queen = PieceBoard(Queen, By);

            // All the occupied squares:
            const BitBoard occupied = ~Empty();

            // Check if the square is under attack by opponent bishops or queens (diagonally).
            const BitBoard diagonalCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occupied)] &
                    (queen | PieceBoard(Bishop, By));

            // Check if the square is under attack by opponent rooks or queens (straight).
            const BitBoard straightCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook, sq, occupied)] &
                    (queen | PieceBoard(Rook, By));

            // For sliding attacks, we must add the square of the attack's origin and all the squares to us from the
            // attack:
            if (diagonalCheck) {
                const Square diagonalCheckSq = ToSquare(diagonalCheck);
                check.Check |= RayTable::Between[sq][diagonalCheckSq] | FromSquare(diagonalCheckSq);
            }

            if (straightCheck) {
                const Square straightCheckSq = ToSquare(straightCheck);
                check.Check |= RayTable::Between[sq][straightCheckSq] | FromSquare(straightCheckSq);

                // In the case where there is more than one check, we must increment the count once more, as it's a
                // double check.
                if (Count(straightCheck) > 1) count++;
            }

            count += static_cast<bool>(diagonalCheck);
            count += static_cast<bool>(straightCheck);

            if (check.Check == BBDefault) check.Check = BBFilled;

            check.DoubleCheck = count > 1;
            return check;
        }

        template<Color We, Color By>
        PinBitBoard Pin() const
        {
            auto pin = PinBitBoard();

            const Square sq = ToSquare(PieceBoard(King, We));

            // All the occupied squares:
            // In this case, we want to let the pins pass through our pieces, since our pieces can move on the pins.
            const BitBoard occupied = ColorBB[By];

            // For queen, we can merge with checks for bishop and rook.
            const BitBoard queen = PieceBoard(Queen, By);

            // Check if the square is under attack by opponent bishops or queens (diagonally).
            const BitBoard diagonalCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occupied)] &
                    (queen | PieceBoard(Bishop, By));

            // Check if the square is under attack by opponent rooks or queens (straight).
            const BitBoard straightCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook, sq, occupied)] &
                    (queen | PieceBoard(Rook, By));

            // Iterate through the attacks and check if the attack is a diagonally pinning one.
            BitBoardIterator iterator(diagonalCheck);
            for (Square attSq = iterator.Value(); attSq != NASQ; attSq = iterator.Value())
                if (const BitBoard possiblePin = RayTable::Between[sq][attSq] | FromSquare(attSq);
                    Count(possiblePin & ColorBB[We]) == 1) pin.Diagonal |= possiblePin;

            // Iterate through the attacks and check if the attack is a straight pinning one.
            iterator = BitBoardIterator(straightCheck);
            for (Square attSq = iterator.Value(); attSq != NASQ; attSq = iterator.Value())
                if (const BitBoard possiblePin = RayTable::Between[sq][attSq] | FromSquare(attSq);
                    Count(possiblePin & ColorBB[We]) == 1) pin.Straight |= possiblePin;

            return pin;
        }

        BitBoard SquareAttackers(const Square sq, const BitBoard occ) const
        {
            BitBoard attackers = AttackTable::Pawn[White][sq] &  PieceBoard( Pawn , Black) |
                                 AttackTable::Pawn[Black][sq] &  PieceBoard( Pawn , White) |
                                 AttackTable::Knight     [sq] & (PieceBoard(Knight, White) | PieceBoard(Knight, Black)) |
                                 AttackTable::King       [sq] & (PieceBoard( King , White) | PieceBoard( King , Black)) ;

            attackers |= AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occ)] &
                    (PieceBoard(Bishop, White) | PieceBoard(Bishop, Black) | PieceBoard(Queen, White) | PieceBoard(Queen, Black));

            attackers |= AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook, sq, occ)] &
                    (PieceBoard( Rook , White) | PieceBoard( Rook , Black) | PieceBoard(Queen, White) | PieceBoard(Queen, Black));

            return attackers;
        }

        template<Piece Piece = NAP>
        [[nodiscard]]
        ::Move CreateMove(const Square from, const Square to, const enum Piece promotion = NAP) const
        {
            assert(from < NASQ && to < NASQ && from != to);
            assert(promotion == NAP || (promotion >= Knight && promotion <= Queen));
            const enum Piece piece = Piece == NAP ? PieceAndColor[from].Piece() : Piece;
            const bool capture = PieceAndColor[to].Piece() != NAP;
            if (promotion != NAP)
                return ::Move(from, to, static_cast<MoveFlag>(8 | (capture ? 4 : 0) | (promotion - Knight)));
            if (piece == Pawn) {
                if (to == EnPassantSquare()) return ::Move(from, to, MoveFlag::EnPassant);
                if ((from ^ to) == 16) return ::Move(from, to, MoveFlag::DoublePush);
            } else if (piece == King && (from == E1 || from == E8) && (from + 2 == to || from - 2 == to)) {
                return ::Move(from, to, to > from ? MoveFlag::KingCastle : MoveFlag::QueenCastle);
            }
            return ::Move(from, to, capture ? MoveFlag::Capture : MoveFlag::Quiet);
        }

        PreviousStateNull Move()
        {
            const PreviousStateNull state(EnPassantSquare());
            HashEnPassant<ZOBRIST>(state.EnPassant);
            SetEnPassant(NASQ);
            CastlingRightAndColorToMove ^= ColorFlipMask;
            if constexpr (Engine) Hash = Zobrist::HashColorFlip<ZOBRIST>(Hash);
            return state;
        }

        void UndoMove(const PreviousStateNull& state)
        {
            SetEnPassant(state.EnPassant);
            HashEnPassant<ZOBRIST>(state.EnPassant);
            CastlingRightAndColorToMove ^= ColorFlipMask;
            if constexpr (Engine) Hash = Zobrist::HashColorFlip<ZOBRIST>(Hash);
        }

        template<MoveType T>
        PreviousState Move(const Square from, const Square to, const Piece promotion = NAP, const size_t threadId = 0)
        {
            return Move<T>(CreateMove(from, to, promotion), threadId);
        }

        template<MoveType T>
        PreviousState Move(const ::Move move, const size_t threadId = 0)
        {
            if constexpr (T & NNUE) Evaluation::PreMove(threadId);

            const Square from = move.From();
            const Square to   = move.To();
            const PieceColor moved = PieceAndColor[from];
            const Piece piece = moved.Piece();
            const Color color = moved.Color();
            const Color opposite = Opposite(color);
            assert(piece != NAP && color == ColorToMove());
            assert(move == CreateMove(from, to, move.Promotion()));
            PreviousState state(moved, PieceAndColor[to], EnPassantSquare(), CastlingRightAndColorToMove, 0);
            if constexpr (Engine && (T & ZOBRIST)) state.Hash = Hash;

            HashEnPassant<T>(state.EnPassant);
            SetEnPassant(NASQ);
            HashCastling<T>();
            CastlingRightAndColorToMove &= CastleMask[from] & CastleMask[to];
            CastlingRightAndColorToMove ^= ColorFlipMask;
            HashCastling<T>();
            if constexpr (Engine && (T & ZOBRIST)) Hash = Zobrist::HashColorFlip<T>(Hash);

            if (move.Capture()) {
                const Square captured = move.EnPassant() ? static_cast<Square>(to ^ 8) : to;
                const Piece capturedPiece = move.EnPassant() ? Pawn : state.CapturedPiece.Piece();
                RemovePiece(capturedPiece, opposite, captured);
                HashPiece<T>(capturedPiece, opposite, captured);
                if constexpr (T & NNUE) Evaluation::Deactivate(capturedPiece, opposite, captured, threadId);
                state.EnPassantCapture = move.EnPassant();
            }

            if (move.Promotion() != NAP) {
                state.PromotedPiece = move.Promotion();
                RemovePiece(Pawn, color, from);
                PlacePiece(move.Promotion(), color, to);
                HashPiece<T>(Pawn, color, from);
                HashPiece<T>(move.Promotion(), color, to);
                if constexpr (T & NNUE) {
                    Evaluation::Deactivate(Pawn, color, from, threadId);
                    Evaluation::Activate(move.Promotion(), color, to, threadId);
                }
            } else {
                TransitionPiece(piece, color, from, to);
                HashPiece<T>(piece, color, from);
                HashPiece<T>(piece, color, to);
                if constexpr (T & NNUE) Evaluation::Transition(piece, color, from, to, threadId);

                if (move.Castling()) {
                    state.CastlingFrom = static_cast<Square>((from & 56) + (to > from ? 7 : 0));
                    state.CastlingTo   = static_cast<Square>((from & 56) + (to > from ? 5 : 3));
                    TransitionPiece(Rook, color, state.CastlingFrom, state.CastlingTo);
                    HashPiece<T>(Rook, color, state.CastlingFrom);
                    HashPiece<T>(Rook, color, state.CastlingTo);
                    if constexpr (T & NNUE)
                        Evaluation::Transition(Rook, color, state.CastlingFrom, state.CastlingTo, threadId);
                } else if (move.DoublePush()) {
                    const auto ep = static_cast<Square>(to ^ 8);
                    if ((T & PERFT) || (AttackTable::Pawn[color][ep] & PieceBoard(Pawn, opposite))) {
                        SetEnPassant(ep);
                        HashEnPassant<T>(ep);
                    }
                }
            }
            UpdateNACBB();
            return state;
        }

        template<MoveType T>
        void UndoMove(const PreviousState& state, const ::Move move, const size_t threadId = 0)
        {
            UndoMove<T>(state, move.From(), move.To(), threadId);
        }

        template<MoveType T>
        void UndoMove(const PreviousState& state, const Square from, const Square to, const size_t threadId = 0)
        {
            if constexpr (T & NNUE) Evaluation::PreUndoMove(threadId);
            CastlingRightAndColorToMove = state.CastlingRightAndColorToMove;
            SetEnPassant(state.EnPassant);
            if constexpr (Engine && (T & ZOBRIST)) Hash = state.Hash;

            const Piece piece = state.MovedPiece.Piece();
            const Color color = state.MovedPiece.Color();
            if (state.PromotedPiece != NAP) {
                RemovePiece(state.PromotedPiece, color, to);
                PlacePiece(Pawn, color, from);
            } else TransitionPiece(piece, color, to, from);

            if (state.CapturedPiece.Piece() != NAP)
                PlacePiece(state.CapturedPiece.Piece(), state.CapturedPiece.Color(), to);
            else if (state.EnPassantCapture)
                PlacePiece(Pawn, Opposite(color), static_cast<Square>(to ^ 8));
            else if (state.CastlingFrom != NASQ)
                TransitionPiece(Rook, color, state.CastlingTo, state.CastlingFrom);
            UpdateNACBB();
        }

        void MoveNative(const Piece piece, const Color color, const Square from,
                        const Piece captured, const Color capturedColor, const Square to)
        {
            if (captured != NAP) RemovePiece(captured, capturedColor, to);
            TransitionPiece(piece, color, from, to);
            UpdateNACBB();
        }

        void EmptyNative(const Piece piece, const Color color, const Square sq)
        {
            if (piece != NAP) RemovePiece(piece, color, sq);
            UpdateNACBB();
        }

        void InsertNative(const Piece piece, const Color color, const Square sq)
        {
            PlacePiece(piece, color, sq);
            UpdateNACBB();
        }

        void UpdateNACBB()
        {
            if constexpr (Engine) ColorBB[NAC] = ~(ColorBB[White] | ColorBB[Black]);
        }

        private:
        [[nodiscard]]
        BitBoard Empty() const
        {
            if constexpr (Engine) return ColorBB[NAC];
            else return ~(ColorBB[White] | ColorBB[Black]);
        }

        void SetEnPassant(const Square sq)
        {
            if constexpr (Engine) EnPassantTarget = sq == NASQ ? BBDefault : FromSquare(sq);
            else EnPassantTarget = sq;
        }

        void RemovePiece(const Piece piece, const Color color, const Square sq)
        {
            assert(sq < NASQ && piece < NAP && color < NAC);
            assert(PieceAndColor[sq].Piece() == piece && PieceAndColor[sq].Color() == color);
            const BitBoard mask = FromSquare(sq);
            BB[Engine ? color : 0][piece] ^= mask;
            ColorBB[color] ^= mask;
            PieceAndColor[sq] = PieceColor(NAP, NAC);
        }

        void PlacePiece(const Piece piece, const Color color, const Square sq)
        {
            assert(sq < NASQ && piece < NAP && color < NAC);
            assert(PieceAndColor[sq].Piece() == NAP);
            const BitBoard mask = FromSquare(sq);
            BB[Engine ? color : 0][piece] |= mask;
            ColorBB[color] |= mask;
            PieceAndColor[sq] = PieceColor(piece, color);
        }

        void TransitionPiece(const Piece piece, const Color color, const Square from, const Square to)
        {
            assert(from < NASQ && to < NASQ && from != to);
            assert(PieceAndColor[from].Piece() == piece && PieceAndColor[from].Color() == color);
            assert(PieceAndColor[to].Piece() == NAP);
            const BitBoard mask = FromSquare(from) | FromSquare(to);
            BB[Engine ? color : 0][piece] ^= mask;
            ColorBB[color] ^= mask;
            PieceAndColor[from] = PieceColor(NAP, NAC);
            PieceAndColor[to] = PieceColor(piece, color);
        }

        template<MoveType T>
        void HashPiece(const Piece piece, const Color color, const Square sq)
        {
            if constexpr (Engine && (T & ZOBRIST)) Hash = Zobrist::HashPiece<T>(Hash, piece, color, sq);
        }

        template<MoveType T>
        void HashEnPassant(const Square sq)
        {
            if constexpr (Engine && (T & ZOBRIST)) Hash = Zobrist::HashEnPassant<T>(Hash, sq);
        }

        template<MoveType T>
        void HashCastling()
        {
            if constexpr (Engine && (T & ZOBRIST)) Hash = Zobrist::HashCastling<T>(Hash, CastlingRights());
        }

        ZobristHash ComputeHash() const
        {
            ZobristHash result = 0;
            BitBoardIterator iterator(~Empty());
            for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value()) {
                const PieceColor pc = PieceAndColor[sq];
                result = Zobrist::HashPiece<ZOBRIST>(result, pc.Piece(), pc.Color(), sq);
            }
            if (ColorToMove() == White) result = Zobrist::HashColorFlip<ZOBRIST>(result);
            result = Zobrist::HashCastling<ZOBRIST>(result, CastlingRights());
            return Zobrist::HashEnPassant<ZOBRIST>(result, EnPassantSquare());
        }

    };

    using Board      = BasicBoard<BoardType::Engine>;
    using PerftBoard = BasicBoard<BoardType::Perft>;

    static_assert(std::is_trivially_copyable_v<Board>);
    static_assert(std::is_trivially_copyable_v<PerftBoard>);
    static_assert(sizeof(PerftBoard) < sizeof(Board));

} // StockDory

#include "PackedBoard.h"

#endif //STOCKDORY_BOARD_H
