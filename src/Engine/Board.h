//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_BOARD_H
#define STOCKDORY_BOARD_H

#include <array>
#include <bit>
#include <cassert>
#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "BlackMagicFactory.h"
#include "Zobrist.h"

namespace StockDory
{

    class BoardState
    {

        constexpr static uint16_t CastlingMask  = 0x000F;
        constexpr static uint16_t SideFlipMask  = 0x0010;
        constexpr static uint16_t EnPassantMask = 0xFF00;
        constexpr static uint8_t  SidePos       =      4;
        constexpr static uint8_t  EnPassantPos  =      8;

        uint16_t Internal = NASQ << EnPassantPos;

        public:
        constexpr BoardState() = default;

        constexpr BoardState(const Side side, const uint8_t castlingRights)
        {
            Internal = NASQ << EnPassantPos | side << SidePos | castlingRights;
        }

        constexpr Side SideToMove() const { return static_cast<Side>((Internal & SideFlipMask) >> SidePos); }

        constexpr uint8_t CastlingRights() const { return Internal & CastlingMask; }

        constexpr Square EnPassantSquare() const { return static_cast<Square>(Internal >> EnPassantPos); }

        constexpr bool CastlingRight(const Side side, const CastlingSide castle) const
        {
            return Internal & 0b1000 >> (side * 2 + castle);
        }

        constexpr void FlipSideToMove() { Internal ^= SideFlipMask; }

        constexpr void RestrictCastling(const uint8_t mask) { Internal &= mask | ~CastlingMask; }

        constexpr void SetEnPassantSquare(const Square sq)
        {
            Internal = (Internal & ~EnPassantMask) | sq << EnPassantPos;
        }

        constexpr void ClearEnPassantSquare() { SetEnPassantSquare(NASQ); }

    };

    template<MakeMode Mode>
    struct BasicPreviousState
    {

        BoardState State   ;
        Piece      Captured;

    };

    template<>
    struct BasicPreviousState<Hashed>
    {

        BoardState State   ;
        Piece      Captured;

        struct
        {

                  ZobristHash     Full ;
                  ZobristHash     Minor;
            Array<ZobristHash, 2> Major;

        } Hash;

    };

    using PreviousState = BasicPreviousState<Hashed>;

    struct PreviousStateNull
    {

        BoardState State;

    };

    struct NoDelta
    {

        static void Remove    (const PieceType, const Side, const Square              ) {}
        static void Insert    (const PieceType, const Side, const Square              ) {}
        static void Transition(const PieceType, const Side, const Square, const Square) {}

    };

    struct GenerationMasks
    {

        CheckBitBoard Check {};
        PinBitBoard   Pin   {};

    };

    struct Castling
    {

        Array<Square  , 2, 2> RookFrom {};
        Array<BitBoard, 2, 2>     Path {};
        Array<BitBoard, 2, 2> KingPath {};

        Array<uint8_t, 64> RightsMask {};

        bool Standard = true;

    };

    // ReSharper disable once StringLiteralTypo
    constexpr std::string_view StartFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    class Board
    {

        constexpr static uint8_t CastlingMask = 0x0F;
        constexpr static uint8_t SideFlipMask = 0x10;

        struct Empty {};

        Array<BitBoard, 2, 6>     BB {};
        Array<BitBoard, 2   > SideBB {};

        Array<Piece, 64> Mailbox {};

              ZobristHash     Hash      = 0;
              ZobristHash     HashMinor = 0;
        Array<ZobristHash, 2> HashMajor  {};

        BoardState State {};

        Castling Castling {};

        public:
        Board() : Board(*Read(StartFen)) {}

        [[nodiscard]]
        static std::expected<Board, std::string_view> FromFen(const std::string_view fen)
        {
            std::expected<Board, std::string_view> board = Read(fen);

            if (board && (board->SideToMove() == White ? board->Checked<Black>() : board->Checked<White>()))
                return std::unexpected("Invalid FEN: the side not to move is in check");

            return board;
        }

        [[nodiscard]]
        std::string Fen(const bool chessFRC = false) const
        {
            std::string result;
            result.reserve(90);

            for (int rank = 7; rank >= 0; rank--) {
                int empty = 0;
                for (int file = 0; file < 8; file++) {
                    const Piece pc = Mailbox[rank * 8 + file];

                    if (pc.Type() == NAPT) {
                        empty++;
                        continue;
                    }

                    if (empty) result += static_cast<char>('0' + std::exchange(empty, 0));

                    result += std::string_view("PNBRQKpnbrqk")[pc.Type() + pc.Side() * 6];
                }

                if (empty) result += static_cast<char>('0' + empty);
                if (rank ) result += '/';
            }

            result += SideToMove() == White ? " w " : " b ";

            if (CastlingRights()) {
                for (const Side         side   : { White   , Black     })
                for (const CastlingSide castle : { KingSide, QueenSide }) {
                    if (!State.CastlingRight(side, castle)) continue;

                    const bool byFile = (chessFRC || !Castling.Standard) && Castling.Path[side][castle] != BBFilled;

                    const char letter = byFile ? static_cast<char>('A' + FileOf(Castling.RookFrom[side][castle]))
                                               : castle == KingSide ? 'K' : 'Q';

                    result += side == White ? letter : static_cast<char>(letter + ('a' - 'A'));
                }
            } else result += '-';

            result += ' ';
            result += EnPassantSquare() == NASQ ? "-" : StockDory::ToString(EnPassantSquare());

            return result;
        }

        [[nodiscard]]
        std::string ToString(const Move move, const bool chessFRC) const
        {
            if (!move) return "0000";

            Square to = move.To();

            if (move.Castling() && (chessFRC || !Castling.Standard)) {
                const Side side = RankOf(move.From()) == 0 ? White : Black;

                const CastlingSide castle = move.Flags() == MoveFlag::KingCastle ? KingSide : QueenSide;

                to = Castling.RookFrom[side][castle];
            }

            std::string result = StockDory::ToString(move.From()) + StockDory::ToString(to);

            if (const PieceType promotion = move.Promotion(); promotion != NAPT)
                result += static_cast<char>(FirstLetter(promotion) + ('a' - 'A'));

            return result;
        }

        [[nodiscard]]
        ZobristHash Zobrist() const { return Hash; }

        [[nodiscard]]
        ZobristHash ZobristMinor() const { return HashMinor; }

        [[nodiscard]]
        ZobristHash ZobristMajor(const Side side) const { return HashMajor[side]; }

        [[nodiscard]]
        Piece operator [](const Square sq) const { return Mailbox[sq]; }

        [[nodiscard]]
        BitBoard operator [](const Side side) const
        {
            assert(side != NAS);

            return SideBB[side];
        }

        [[nodiscard]]
        BitBoard Occupied() const { return SideBB[White] | SideBB[Black]; }

        template<Side S>
        [[nodiscard]]
        BitBoard PieceBoard(const PieceType type) const { return PieceBoard(type, S); }

        [[nodiscard]]
        BitBoard PieceBoard(const PieceType type, const Side side) const
        {
            assert(type != NAPT && side != NAS);

            return BB[side][type];
        }

        template<Side S>
        [[nodiscard]]
        bool HasMajorMaterial() const
        {
            return PieceBoard(Knight, S) ||
                   PieceBoard(Bishop, S) ||
                   PieceBoard(Rook  , S) ||
                   PieceBoard(Queen , S)  ;
        }

        [[nodiscard]]
        Side SideToMove() const { return State.SideToMove(); }

        [[nodiscard]]
        uint8_t CastlingRights() const { return State.CastlingRights(); }

        template<Side S>
        [[nodiscard]]
        bool CastlingRight(const CastlingSide castle) const
        {
            static_assert(S != NAS);

            return State.CastlingRight(S, castle);
        }

        [[nodiscard]]
        const StockDory::Castling& GetCastling() const { return Castling; }

        [[nodiscard]]
        Square EnPassantSquare() const { return State.EnPassantSquare(); }

        template<Side S>
        bool Checked() const
        {
            constexpr Side by = Opposite(S);

            const Square king = ToSquare(PieceBoard(King, S));

            if (AttackTable::Pawn[S][king] & PieceBoard(Pawn  , by)) return true;
            if (AttackTable::Knight [king] & PieceBoard(Knight, by)) return true;

            const BitBoard occupied = Occupied();

            const BitBoard queen = PieceBoard(Queen, by);

            if (AttackTable::Bishop(king, occupied) & (queen | PieceBoard(Bishop, by))) return true;
            if (AttackTable::Rook  (king, occupied) & (queen | PieceBoard(Rook  , by))) return true;

            return AttackTable::King[king] & PieceBoard(King, by);
        }

        template<Side S>
        CheckBitBoard Check() const
        {
            uint8_t count =               0;
            auto    check = CheckBitBoard();

            const Square sq = ToSquare(PieceBoard(King, S));

            const BitBoard   pawnCheck = AttackTable::Pawn[S][sq] & PieceBoard(Pawn  , Opposite(S));
            const BitBoard knightCheck = AttackTable::Knight [sq] & PieceBoard(Knight, Opposite(S));

            check.Check |=   pawnCheck;
            check.Check |= knightCheck;

            count += static_cast<bool>(  pawnCheck);
            count += static_cast<bool>(knightCheck);

            const BitBoard queen = PieceBoard(Queen, Opposite(S));

            const BitBoard occupied = Occupied();

            const BitBoard diagonalCheck =
                AttackTable::Bishop(sq, occupied) & (queen | PieceBoard(Bishop, Opposite(S)));

            const BitBoard straightCheck =
                AttackTable::Rook  (sq, occupied) & (queen | PieceBoard(Rook  , Opposite(S)));

            if (diagonalCheck) {
                const Square diagonalCheckSq = ToSquare(diagonalCheck);
                check.Check |= RayTable::Between[sq][diagonalCheckSq] | FromSquare(diagonalCheckSq);
            }

            if (straightCheck) {
                const Square straightCheckSq = ToSquare(straightCheck);
                check.Check |= RayTable::Between[sq][straightCheckSq] | FromSquare(straightCheckSq);

                if (Count(straightCheck) > 1) count++;
            }

            count += static_cast<bool>(diagonalCheck);
            count += static_cast<bool>(straightCheck);

            if (check.Check == BBDefault) check.Check = BBFilled;

            check.DoubleCheck = count > 1;
            return check;
        }

        template<Side S>
        PinBitBoard Pin() const
        {
            auto pin = PinBitBoard();

            const Square king = ToSquare(PieceBoard(King, S));

            const BitBoard occupied = SideBB[Opposite(S)];

            const BitBoard queen = PieceBoard(Queen, Opposite(S));

            const BitBoard diagonal = AttackTable::Bishop(king, occupied) & (queen | PieceBoard(Bishop, Opposite(S)));
            const BitBoard straight = AttackTable::Rook  (king, occupied) & (queen | PieceBoard(Rook  , Opposite(S)));

            for (const Square pinner : Squares(diagonal))
                if (const BitBoard ray = RayTable::Between[king][pinner] | FromSquare(pinner);
                    Count(ray & SideBB[S]) == 1) pin.Diagonal |= ray;

            for (const Square pinner : Squares(straight))
                if (const BitBoard ray = RayTable::Between[king][pinner] | FromSquare(pinner);
                    Count(ray & SideBB[S]) == 1) pin.Straight |= ray;

            return pin;
        }

        template<Side S>
        GenerationMasks Masks() const { return { .Check = Check<S>(), .Pin = Pin<S>() }; }

        BitBoard Attackers(const Square sq, const BitBoard occupied) const
        {
            BitBoard attackers = AttackTable::Pawn[White][sq] &  PieceBoard( Pawn , Black)  |
                                 AttackTable::Pawn[Black][sq] &  PieceBoard( Pawn , White)  |

                                 AttackTable::Knight     [sq] & (PieceBoard(Knight, White)  |
                                                                 PieceBoard(Knight, Black)) |

                                 AttackTable::King       [sq] & (PieceBoard( King , White)  |
                                                                 PieceBoard( King , Black)) ;

            attackers |= AttackTable::Bishop(sq, occupied) &
                    (PieceBoard(Bishop, White) |
                     PieceBoard(Bishop, Black) |
                     PieceBoard(Queen , White) |
                     PieceBoard(Queen , Black));

            attackers |= AttackTable::Rook  (sq, occupied) &
                    (PieceBoard( Rook , White) |
                     PieceBoard( Rook , Black) |
                     PieceBoard(Queen , White) |
                     PieceBoard(Queen , Black));

            return attackers;
        }

        [[nodiscard]]
        Move ConstructMove(const Square from, const Square to, const PieceType promotion = NAPT) const
        {
            assert(from < NASQ && to < NASQ && from != to);
            assert(promotion == NAPT || (promotion >= Knight && promotion <= Queen));

            const PieceType type = Mailbox[from].Type();

            const bool capture = Mailbox[to].Type() != NAPT;

            if (promotion != NAPT)
                return Move(from, to, static_cast<MoveFlag>(8 | (capture ? 4 : 0) | (promotion - Knight)));
            if (type == Pawn) {
                if (to == EnPassantSquare()) return Move(from, to, MoveFlag::EnPassant );
                if ((from ^ to) == 16)       return Move(from, to, MoveFlag::DoublePush);
            } else if (type == King) {
                const Side side = Mailbox[from].Side();

                for (const CastlingSide castle : { KingSide, QueenSide }) {
                    if (!State.CastlingRight(side, castle) || Castling.Path[side][castle] == BBFilled) continue;

                    const Square   kingTo = MakeSquare(castle == KingSide ? 6 : 2, RankOf(from));
                    const MoveFlag flag   = castle == KingSide ? MoveFlag::KingCastle : MoveFlag::QueenCastle;

                    if (to == Castling.RookFrom[side][castle]            ) return Move(from, kingTo, flag);
                    if (to == kingTo && !Get(AttackTable::King[from], to)) return Move(from, kingTo, flag);
                }
            }

            return Move(from, to, capture ? MoveFlag::Capture : MoveFlag::Base);
        }

        template<MakeMode Mode, typename Delta = NoDelta>
        BasicPreviousState<Mode> MakeMove(const Move move, Delta&& delta = {})
        {
            const Square from = move.From();
            const Square to   = move.  To();

            const Side side     = SideToMove();
            const Side opposite = Opposite(side);

            const PieceType type = Mailbox[from].Type();

            BasicPreviousState<Mode> state;

            state.State    = State;
            state.Captured = Mailbox[to];

            if constexpr (Mode == Hashed) state.Hash = { Hash, HashMinor, HashMajor };

            BoardState next = State;

            HashEnPassant<Mode>(next.EnPassantSquare());
            HashCastling <Mode>(next.CastlingRights ());

            next.ClearEnPassantSquare(                                                   );
            next.RestrictCastling    (Castling.RightsMask[from] & Castling.RightsMask[to]);
            next.FlipSideToMove      (                                                   );

            HashCastling<Mode>(next.CastlingRights());
            HashSideFlip<Mode>(                     );

            if (move.Castling()) {
                const CastlingSide castle = move.Flags() == MoveFlag::KingCastle ? KingSide : QueenSide;

                const Square rookFrom = Castling.RookFrom[side][castle];
                const Square rookTo   = MakeSquare(castle == KingSide ? 5 : 3, RankOf(from));

                state.Captured = {};

                RemovePiece<Mode>(King, side,     from);
                RemovePiece<Mode>(Rook, side, rookFrom);
                PlacePiece <Mode>(King, side,       to);
                PlacePiece <Mode>(Rook, side,   rookTo);

                if (    from !=     to) delta.Transition(King, side,     from,     to);
                if (rookFrom != rookTo) delta.Transition(Rook, side, rookFrom, rookTo);
            } else {
                if (move.Capture()) {
                    const Square    captured     = move.EnPassant() ? static_cast<Square>(to ^ 8) : to;
                    const PieceType capturedType = move.EnPassant() ? Pawn : state.Captured.Type();

                    RemovePiece<Mode>(capturedType, opposite, captured);

                    delta.Remove(capturedType, opposite, captured);
                }

                if (move.Promotion() != NAPT) {
                    RemovePiece<Mode>(Pawn            , side, from);
                    PlacePiece <Mode>(move.Promotion(), side,   to);

                    delta.Remove(Pawn            , side, from);
                    delta.Insert(move.Promotion(), side,   to);
                } else {
                    TransitionPiece<Mode>(type, side, from, to);

                    delta.Transition(type, side, from, to);

                    if (move.DoublePush()) {
                        const auto enPassantSquare = static_cast<Square>(to ^ 8);

                        if (Mode != Hashed || EnPassantCapturable(enPassantSquare, opposite)) {
                            next.SetEnPassantSquare(enPassantSquare);

                            HashEnPassant<Mode>(enPassantSquare);
                        }
                    }
                }
            }

            State = next;

            return state;
        }

        template<MakeMode Mode>
        void UndoMove(const BasicPreviousState<Mode>& state, const Move move)
        {
            const Square from = move.From();
            const Square to   = move.  To();

            State = state.State;

            if constexpr (Mode == Hashed) {
                Hash      = state.Hash.Full ;
                HashMinor = state.Hash.Minor;
                HashMajor = state.Hash.Major;
            }

            const Side side     = SideToMove();
            const Side opposite = Opposite(side);

            if (move.Castling()) {
                const CastlingSide castle = move.Flags() == MoveFlag::KingCastle ? KingSide : QueenSide;

                const Square rookFrom = Castling.RookFrom[side][castle];
                const Square rookTo   = MakeSquare(castle == KingSide ? 5 : 3, RankOf(from));

                RemovePiece<Plain>(King, side,       to);
                RemovePiece<Plain>(Rook, side,   rookTo);
                PlacePiece <Plain>(King, side,     from);
                PlacePiece <Plain>(Rook, side, rookFrom);
            } else {
                if (move.Promotion() != NAPT) {
                    RemovePiece<Plain>(move.Promotion(), side,   to);
                    PlacePiece <Plain>(Pawn            , side, from);
                } else TransitionPiece<Plain>(Mailbox[to].Type(), side, to, from);

                if (move.Capture()) {
                    const Piece captured = state.Captured;

                    if (move.EnPassant()) PlacePiece<Plain>(Pawn, opposite, static_cast<Square>(to ^ 8));
                    else                  PlacePiece<Plain>(captured.Type(), captured.Side(), to);
                }
            }
        }

        PreviousStateNull MakeNullMove()
        {
            const PreviousStateNull state { .State = State };

            BoardState next = State;

            HashEnPassant<Hashed>(next.EnPassantSquare());

            next.ClearEnPassantSquare();
            next.FlipSideToMove      ();

            HashSideFlip<Hashed>();

            State = next;

            return state;
        }

        void UndoNullMove(const PreviousStateNull& state)
        {
            State = state.State;

            HashEnPassant<Hashed>(State.EnPassantSquare());
            HashSideFlip <Hashed>();
        }

        private:
        [[nodiscard]]
        bool EnPassantCapturable(const Square enPassantSquare, const Side side) const
        {
            const Side pushed = Opposite(side);

            const BitBoard candidates = AttackTable::Pawn[pushed][enPassantSquare] & PieceBoard(Pawn, side);
            if (!candidates) return false;

            const Square   king     = ToSquare(PieceBoard(King, side));
            const BitBoard queen    = PieceBoard(Queen, pushed);
            const BitBoard straight = queen | PieceBoard(Rook  , pushed);
            const BitBoard diagonal = queen | PieceBoard(Bishop, pushed);
            const BitBoard occupied = Occupied() & ~FromSquare(static_cast<Square>(enPassantSquare ^ 8));

            for (const Square sq : Squares(candidates)) {
                const BitBoard after = (occupied & ~FromSquare(sq)) | FromSquare(enPassantSquare);

                if (!(AttackTable::Rook  (king, after) & straight) &&
                    !(AttackTable::Bishop(king, after) & diagonal))
                    return true;
            }

            return false;
        }

        template<MakeMode Mode>
        void RemovePiece(const PieceType type, const Side side, const Square sq)
        {
            assert(sq < NASQ && type < NAPT && side < NAS);
            assert(Mailbox[sq].Type() == type && Mailbox[sq].Side() == side);

            const BitBoard mask = FromSquare(sq);

                BB[side][type] ^= mask;
            SideBB[side]       ^= mask;

            Mailbox[sq] = Piece(NAPT, NAS);

            HashPiece<Mode>(type, side, sq);
        }

        template<MakeMode Mode>
        void PlacePiece(const PieceType type, const Side side, const Square sq)
        {
            assert(sq < NASQ && type < NAPT && side < NAS);
            assert(Mailbox[sq].Type() == NAPT);

            const BitBoard mask = FromSquare(sq);

                BB[side][type] |= mask;
            SideBB[side]       |= mask;

            Mailbox[sq] = Piece(type, side);

            HashPiece<Mode>(type, side, sq);
        }

        template<MakeMode Mode>
        void TransitionPiece(const PieceType type, const Side side, const Square from, const Square to)
        {
            assert(from < NASQ && to < NASQ && from != to);
            assert(Mailbox[from].Type() == type && Mailbox[from].Side() == side);
            assert(Mailbox[to].Type() == NAPT);

            const BitBoard mask = FromSquare(from) | FromSquare(to);

            BB    [side][type] ^= mask;
            SideBB[side]       ^= mask;

            Mailbox[from] = Piece(NAPT, NAS );
            Mailbox[ to ] = Piece(type, side);

            HashPiece<Mode>(type, side, from);
            HashPiece<Mode>(type, side,  to );
        }

        template<MakeMode Mode>
        void HashPiece(const PieceType type, const Side side, const Square sq)
        {
            if (Mode == Hashed) {
                Hash = Zobrist::HashPiece<Mode>(Hash, type, side, sq);

                if (type == Pawn) HashMinor       = Zobrist::HashPiece<Mode>(HashMinor      , type, side, sq);
                else              HashMajor[side] = Zobrist::HashPiece<Mode>(HashMajor[side], type, side, sq);
            }
        }

        template<MakeMode Mode>
        void HashEnPassant(const Square sq)
        {
            if (Mode == Hashed) Hash = Zobrist::HashEnPassant<Mode>(Hash, sq);
        }

        template<MakeMode Mode>
        void HashCastling(const uint8_t castlingRights)
        {
            if (Mode == Hashed) Hash = Zobrist::HashCastling<Mode>(Hash, castlingRights);
        }

        template<MakeMode Mode>
        void HashSideFlip()
        {
            if (Mode == Hashed) Hash = Zobrist::HashSideFlip<Mode>(Hash);
        }

        ZobristHash ComputeHashMinor() const
        {
            ZobristHash result = 0;

            for (const Square sq : Squares(PieceBoard(Pawn, White) | PieceBoard(Pawn, Black)))
                result = Zobrist::HashPiece<Hashed>(result, Pawn, Mailbox[sq].Side(), sq);

            return result;
        }

        ZobristHash ComputeHashMajor(const Side side) const
        {
            ZobristHash result = 0;

            for (const Square sq : Squares(SideBB[side] & ~PieceBoard(Pawn, side)))
                result = Zobrist::HashPiece<Hashed>(result, Mailbox[sq].Type(), side, sq);

            return result;
        }

        explicit Board(const Empty) {}

        static std::expected<Board, std::string_view> Read(const std::string_view fen)
        {
            Board board { Empty {} };

            size_t offset = 0;
            Array<std::string_view, 4> fields {};

            for (auto& field : fields) {
                offset = fen.find_first_not_of(' ', offset);

                if (offset == std::string_view::npos) return std::unexpected("Incomplete FEN");

                const size_t end = fen.find(' ', offset);
                field = fen.substr(offset, end == std::string_view::npos ? end : end - offset);
                offset = end;
            }

            int rank = 7;
            int file = 0;
            for (const char value : fields[0]) {
                if (value == '/') {
                    if (file != 8 || rank == 0) return std::unexpected("Invalid FEN rank");

                    rank--;
                    file = 0;

                    continue;
                }

                if (value >= '1' && value <= '8') {
                    file += value - '0';

                    if (file > 8) return std::unexpected("Invalid FEN rank");

                    continue;
                }

                if (file >= 8) return std::unexpected("Invalid FEN square");

                const size_t index = std::string_view("PNBRQKpnbrqk").find(value);
                if (index == std::string_view::npos) return std::unexpected("Invalid FEN piece");

                const auto type = static_cast<PieceType>(index % 6);
                const Side side = index < 6 ? White : Black;

                board.PlacePiece<Plain>(type, side, static_cast<Square>(rank * 8 + file++));
            }

            if (rank != 0 || file != 8) return std::unexpected("Invalid FEN placement");

            if (fields[1] != "w" && fields[1] != "b") return std::unexpected("Invalid FEN side");

            const Side side = fields[1] == "w" ? White : Black;

            if (fields[3] != "-" && (fields[3].size() != 2   ||
                                     fields[3][0]      < 'a' ||
                                     fields[3][0]      > 'h' ||
                                     fields[3][1]     != (side == White ? '6' : '3')))
                return std::unexpected("Invalid FEN en passant square");

            if (Count(board.PieceBoard(King, White)) != 1 || Count(board.PieceBoard(King, Black)) != 1)
                return std::unexpected("Invalid FEN: each side needs exactly one king");

            const BitBoard pawns = board.PieceBoard(Pawn, White) | board.PieceBoard(Pawn, Black);

            if (pawns & (RayTable::Rank[0] | RayTable::Rank[7]))
                return std::unexpected("Invalid FEN: a pawn on the first or last rank");

            const std::expected<uint8_t, std::string_view> castlingRights = board.SetUpCastling(fields[2]);

            if (!castlingRights) return std::unexpected(castlingRights.error());

            board.State = BoardState(side, *castlingRights);

            if (fields[3] != "-") {
                const Square enPassantSquare = MakeSquare(fields[3][0] - 'a', fields[3][1] - '1');
                const Piece  pushed          = board[static_cast<Square>(enPassantSquare ^ 8)];

                if (board[enPassantSquare].Type() == NAPT && pushed.Type() == Pawn && pushed.Side() != side &&
                    board.EnPassantCapturable(enPassantSquare, side))
                    board.State.SetEnPassantSquare(enPassantSquare);
            }

            board.Hash             = board.ComputeHash     (     );
            board.HashMinor        = board.ComputeHashMinor(     );
            board.HashMajor[White] = board.ComputeHashMajor(White);
            board.HashMajor[Black] = board.ComputeHashMajor(Black);

            return board;
        }

        std::expected<uint8_t, std::string_view> SetUpCastling(const std::string_view field)
        {
            uint8_t rights = 0;

            Array<Square, 2, 2> rooks;
            Fill(rooks, NASQ);

            for (const char letter : field == "-" ? std::string_view() : field) {
                const Side side  = letter >= 'a' ? Black : White;
                const char upper = side == Black ? static_cast<char>(letter - ('a' - 'A')) : letter;

                const uint8_t rank = side == White ? 0 : 7;

                const Square   king       = ToSquare(PieceBoard(King, side));
                const BitBoard candidates = PieceBoard(Rook, side) & RayTable::Rank[rank];

                Square rook = NASQ;

                if (upper == 'K' || upper == 'Q') {
                    const BitBoard outer = candidates & (upper == 'K' ? ~(2 * FromSquare(king) - 1)
                                                                      :       FromSquare(king) - 1 );

                    if (RankOf(king) == rank && outer)
                        rook = upper == 'K' ? static_cast<Square>(63 - std::countl_zero(outer)) : ToSquare(outer);
                } else if (upper >= 'A' && upper <= 'H') {
                    rook = MakeSquare(upper - 'A', rank);

                    if (RankOf(king) != rank || !Get(candidates, rook))
                        return std::unexpected("Invalid FEN castling rights");
                } else  return std::unexpected("Invalid FEN castling rights");

                const CastlingSide castle = upper == 'K' ? KingSide  :
                                            upper == 'Q' ? QueenSide : rook > king ? KingSide : QueenSide;

                const uint8_t right = 0b1000 >> (side * 2 + castle);

                if (rights & right) return std::unexpected("Invalid FEN castling rights");

                rights             |= right;
                rooks[side][castle] = rook;
            }

            Castling.RightsMask.fill(CastlingMask | SideFlipMask);

            for (const Side side : { White, Black }) {
                const uint8_t rank = side == White ? 0 : 7;

                for (const CastlingSide castle : { KingSide, QueenSide }) {
                    const bool possible = rooks[side][castle] != NASQ;

                    const Square corner = MakeSquare(castle == KingSide ? 7 : 0, rank);

                    const Square king     = possible ? ToSquare(PieceBoard(King, side)) : MakeSquare(4, rank);
                    const Square rookFrom = possible ? rooks[side][castle]              :              corner;

                    const Square kingTo = MakeSquare(castle == KingSide ? 6 : 2, rank);
                    const Square rookTo = MakeSquare(castle == KingSide ? 5 : 3, rank);

                    const BitBoard kingPath = RankPath(king    , kingTo);
                    const BitBoard rookPath = RankPath(rookFrom, rookTo);

                    Castling.RookFrom[side][castle] = rookFrom;
                    Castling.KingPath[side][castle] = kingPath;

                    Castling.Path[side][castle] = possible ?
                                                  (kingPath | rookPath) & ~(FromSquare(king) | FromSquare(rookFrom)) :
                                                  BBFilled;

                    const uint8_t right = 0b1000 >> (side * 2 + castle);

                    Castling.RightsMask[king    ] &= ~right;
                    Castling.RightsMask[rookFrom] &= ~right;

                    if (possible && (FileOf(king) != 4 || rookFrom != corner)) Castling.Standard = false;
                }
            }

            return rights;
        }

        constexpr static BitBoard RankPath(const Square from, const Square to)
        {
            const BitBoard range = from < to ? (2ULL << to) - (1ULL << from) : (2ULL << from) - (1ULL << to);

            return (range & ~FromSquare(from)) | FromSquare(to);
        }

        ZobristHash ComputeHash() const
        {
            ZobristHash result = 0;

            for (const Square sq : Squares(Occupied())) {
                const Piece pc = Mailbox[sq];
                result = Zobrist::HashPiece<Hashed>(result, pc.Type(), pc.Side(), sq);
            }

            if (SideToMove() == White) result = Zobrist::HashSideFlip<Hashed>(result);

            result = Zobrist::HashCastling<Hashed>(result, CastlingRights());

            return Zobrist::HashEnPassant<Hashed>(result, EnPassantSquare());
        }

    };

    static_assert(std::is_trivially_copyable_v<Board>);

} // StockDory

#endif //STOCKDORY_BOARD_H
