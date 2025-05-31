//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_BOARD_H
#define STOCKDORY_BOARD_H

#include <array>
#include <cassert>
#include <iostream>
#include <string>

#include "Type/BitBoard.h"
#include "Type/CheckBitBoard.h"
#include "Type/Color.h"
#include "Type/Piece.h"
#include "Type/PieceColor.h"
#include "Type/PinBitBoard.h"
#include "Type/PreviousState.h"
#include "Type/Zobrist.h"

#include "Template/MoveType.h"

#include "Move/AttackTable.h"
#include "Move/BlackMagicFactory.h"
#include "Move/RayTable.h"

#include "../External/strutil.h"

#include "../Engine/Evaluation.h"

namespace StockDory
{

    class Board
    {

        constexpr static uint8_t CastlingMask     = 0xF;
        constexpr static uint8_t WhiteKCastleMask = 0x8;
        constexpr static uint8_t WhiteQCastleMask = 0x4;
        constexpr static uint8_t BlackKCastleMask = 0x2;
        constexpr static uint8_t BlackQCastleMask = 0x1;

        constexpr static uint8_t ColorFlipMask = 0x10;

        constexpr static std::array<uint8_t, 3> ColorCastleMask {
            WhiteKCastleMask | WhiteQCastleMask,
            BlackKCastleMask | BlackQCastleMask,
            0
        };

        std::array<std::array<BitBoard, 7>, 3> BB {};

        std::array<PieceColor, 64> PieceAndColor {};

        std::array<BitBoard, 3> ColorBB {};

        // [COLOR TO MOVE] [WHITE KING CASTLE] [WHITE QUEEN CASTLE] [BLACK KING CASTLE] [BLACK QUEEN CASTLE]
        // [    4 BITS   ] [      1 BIT      ] [       1 BIT      ] [      1 BIT      ] [       1 BIT      ]
        uint8_t CastlingRightAndColorToMove = 0;

        BitBoard EnPassantTarget = BBDefault;

        ZobristHash Hash = 0;

        public:
        Board() : Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}

        Board(const std::string& fen)
        {
            constexpr auto none = PieceColor(NAP, NAC);
            std::ranges::fill(PieceAndColor, none);

            for (uint8_t i = 0; i < 3; i++)
                std::ranges::fill(BB[i], BBDefault);

            const std::vector<std::string> splitFen = strutil::split(fen, " ");

            assert(splitFen.size() == 6);

            std::vector<std::string> splitPosition = strutil::split(splitFen[0], "/");
            std::ranges::reverse(splitPosition);

            assert(splitPosition.size() == 8);

            for (uint8_t v = 0; v < 8; v++) {
                std::string& rankStr = splitPosition[v];
                uint8_t      h       = 0;
                for (const char p: rankStr) {
                    if (isdigit(p)) {
                        h += static_cast<uint8_t>(p - 48);
                        continue;
                    }

                    Color color = Black;

                    if (isupper(p)) color = White;

                    Piece piece = NAP;
                    switch (tolower(p)) {
                        case 'p':
                            piece = Pawn;
                            break;
                        case 'n':
                            piece = Knight;
                            break;
                        case 'b':
                            piece = Bishop;
                            break;
                        case 'r':
                            piece = Rook;
                            break;
                        case 'q':
                            piece = Queen;
                            break;
                        case 'k':
                            piece = King;
                            break;
                        default:;
                    }

                    uint8_t    idx = v * 8 + h;
                    const auto sq  = static_cast<Square>(idx);

                    Set<true>(BB[color][piece], sq);

                    PieceAndColor[idx] = PieceColor(piece, color);

                    if (piece == NAP) std::cout << "ERROR" << std::endl;

                    Hash = Zobrist::HashPiece<ZOBRIST>(Hash, piece, color, sq);

                    h++;
                }
            }

            if (splitFen[1][0] == 'w') {
                CastlingRightAndColorToMove = White << 4;
                Hash                        = Zobrist::HashColorFlip<ZOBRIST>(Hash);
            } else {
                CastlingRightAndColorToMove = Black << 4;
            }

            const std::string& castlingData = splitFen[2];
            CastlingRightAndColorToMove |= castlingData.find('K') != std::string::npos ? WhiteKCastleMask : 0x0;
            CastlingRightAndColorToMove |= castlingData.find('Q') != std::string::npos ? WhiteQCastleMask : 0x0;
            CastlingRightAndColorToMove |= castlingData.find('k') != std::string::npos ? BlackKCastleMask : 0x0;
            CastlingRightAndColorToMove |= castlingData.find('q') != std::string::npos ? BlackQCastleMask : 0x0;

            Hash = Zobrist::HashCastling<ZOBRIST>(Hash, CastlingRightAndColorToMove & CastlingMask);

            EnPassantTarget     = BBDefault;
            if (const std::string& epData = splitFen[3]; epData.length() == 2) {
                if (const Square epSq = FromString(epData);
                    AttackTable::Pawn[Opposite(ColorToMove())][epSq] & BB[ColorToMove()][Pawn]) {
                    EnPassantTarget = FromSquare(epSq);
                    Hash            = Zobrist::HashEnPassant<ZOBRIST>(Hash, epSq);
                }
            }

            ColorBB[White] = BBDefault;
            ColorBB[Black] = BBDefault;
            for (Piece p = Pawn; p != NAP; p = Next(p)) {
                ColorBB[White] |= BB[White][p];
                ColorBB[Black] |= BB[Black][p];
            }
            ColorBB[NAC] = ~(ColorBB[White] | ColorBB[Black]);
        }

        void LoadForEvaluation(const size_t threadId = 0) const
        {
            Evaluation::ResetNetworkState(threadId);

            for (Square sq = A1; sq < NASQ; sq = Next(sq)) {
                PieceColor pc = PieceAndColor[sq];
                if (pc.Piece() == NAP || pc.Color() == NAC) continue;

                Evaluation::Activate(pc.Piece(), pc.Color(), sq, threadId);
            }
        }

        std::string Fen() const
        {
            std::array<std::string, 8> fenRank;

            for (uint8_t v = 0; v < 8; v++) {
                std::stringstream rankStr;
                uint8_t           e = 0;
                for (uint8_t h = 0; h < 8; h++) {
                    const PieceColor pc = PieceAndColor[v * 8 + h];

                    if (pc.Piece() == NAP) {
                        e++;

                        if (h == 7) {
                            rankStr << static_cast<uint16_t>(e);
                            e = 0;
                        }
                        continue;
                    }

                    if (e != 0) {
                        rankStr << static_cast<uint16_t>(e);
                        e = 0;
                    }

                    char p = ' ';

                    switch (pc.Piece()) {
                        case Pawn:
                            p = 'p';
                            break;
                        case Knight:
                            p = 'n';
                            break;
                        case Bishop:
                            p = 'b';
                            break;
                        case Rook:
                            p = 'r';
                            break;
                        case Queen:
                            p = 'q';
                            break;
                        case King:
                            p = 'k';
                            break;
                        case NAP:
                            break;
                    }

                    if (pc.Color() == White) p = static_cast<char>(toupper(p));

                    rankStr << p;
                }

                fenRank[v] = rankStr.str();
            }

            std::stringstream fen;
            for (uint8_t v = 0; v < 8; v++) {
                fen << fenRank[7 - v];
                if (v != 7) fen << '/';
            }

            fen << ' ';
            fen << (ColorToMove() == White ? 'w' : 'b');
            fen << ' ';

            if     (CastlingRightAndColorToMove &     CastlingMask) {
                if (CastlingRightAndColorToMove & WhiteKCastleMask) fen << 'K';
                if (CastlingRightAndColorToMove & WhiteQCastleMask) fen << 'Q';
                if (CastlingRightAndColorToMove & BlackKCastleMask) fen << 'k';
                if (CastlingRightAndColorToMove & BlackQCastleMask) fen << 'q';
            } else fen << '-';

            fen << ' ';
            if (EnPassantSquare() != NASQ) fen << ToString(ToSquare(EnPassantTarget));
            else fen << '-';

            // Implement half and full move clocks.
            fen << ' ';
            fen << '0';
            fen << ' ';
            fen << '1';

            return fen.str();
        }

        ZobristHash Zobrist() const
        {
            return Hash;
        }

        PieceColor operator [](const Square sq) const
        {
            return PieceAndColor[sq];
        }

        BitBoard operator [](const Color c) const
        {
            return ColorBB[c];
        }

        template<Color Color>
        BitBoard PieceBoard(const Piece p) const
        {
            assert(p != NAP);
            assert(Color != NAC);

            return BB[Color][p];
        }

        [[nodiscard]]
        BitBoard PieceBoard(const Piece p, const Color c) const
        {
            assert(p != NAP);
            assert(c != NAC);

            return BB[c][p];
        }

        Color ColorToMove() const
        {
            return static_cast<Color>(CastlingRightAndColorToMove >> 4);
        }

        template<Color Color>
        bool CastlingRightK() const
        {
            if (Color == White) return CastlingRightAndColorToMove & WhiteKCastleMask;
            if (Color == Black) return CastlingRightAndColorToMove & BlackKCastleMask;

            throw std::invalid_argument("Invalid color");
        }

        template<Color Color>
        bool CastlingRightQ() const
        {
            if (Color == White) return CastlingRightAndColorToMove & WhiteQCastleMask;
            if (Color == Black) return CastlingRightAndColorToMove & BlackQCastleMask;

            throw std::invalid_argument("Invalid color");
        }

        BitBoard EnPassant() const
        {
            return EnPassantTarget;
        }

        Square EnPassantSquare() const
        {
            return ToSquare(EnPassantTarget);
        }

        template<Color We>
        bool Checked() const
        {
            constexpr Color by = Opposite(We);

            const Square king = ToSquare(BB[We][King]);

            if (AttackTable::Pawn[We][king] & BB[by][Pawn]) return true;

            if (AttackTable::Knight[king] & BB[by][Knight]) return true;

            const BitBoard occupied = ~ColorBB[NAC];
            const BitBoard queen    = BB[by][Queen];

            if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, king, occupied)] &
                (queen | BB[by][Bishop]))
                return true;

            if (AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook  , king, occupied)] &
                (queen | BB[by][ Rook ]))
                return true;

            return AttackTable::King[king] & BB[by][King];
        }

        template<Color By>
        CheckBitBoard Check() const
        {
            uint8_t count = 0;
            auto    check = CheckBitBoard();

            const Square sq = ToSquare(BB[Opposite(By)][King]);

            // Check if the square is under attack by opponent knights or pawns.
            const BitBoard pawnCheck   = AttackTable::Pawn[Opposite(By)][sq] & BB[By][Pawn];
            const BitBoard knightCheck = AttackTable::Knight[sq] & BB[By][Knight];

            // If the square is under attack by a pawn or knight, add it our checks.
            check.Check |= pawnCheck;
            check.Check |= knightCheck;

            // Increment the count if there are checks.
            count += static_cast<bool>(pawnCheck);
            count += static_cast<bool>(knightCheck);

            // Check if the square is under attack by opponent bishops, rooks, or queens.
            // For queen, we can merge with checks for bishop and rook.
            const BitBoard queen = BB[By][Queen];

            // All the occupied squares:
            const BitBoard occupied = ~ColorBB[NAC];

            // Check if the square is under attack by opponent bishops or queens (diagonally).
            const BitBoard diagonalCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occupied)] &
                    (queen | BB[By][Bishop]);

            // Check if the square is under attack by opponent rooks or queens (straight).
            const BitBoard straightCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook, sq, occupied)] &
                    (queen | BB[By][Rook]);

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

            const Square sq = ToSquare(BB[We][King]);

            // All the occupied squares:
            // In this case, we want to let the pins pass through our pieces, since our pieces can move on the pins.
            const BitBoard occupied = ColorBB[By];

            // For queen, we can merge with checks for bishop and rook.
            const BitBoard queen = BB[By][Queen];

            // Check if the square is under attack by opponent bishops or queens (diagonally).
            const BitBoard diagonalCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occupied)] &
                    (queen | BB[By][Bishop]);

            // Check if the square is under attack by opponent rooks or queens (straight).
            const BitBoard straightCheck =
                    AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook, sq, occupied)] &
                    (queen | BB[By][Rook]);

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
            BitBoard attackers = AttackTable::Pawn[White][sq] &  BB[Black][ Pawn ] |
                                 AttackTable::Pawn[Black][sq] &  BB[White][ Pawn ] |
                                 AttackTable::Knight     [sq] & (BB[White][Knight] | BB[Black][Knight]) |
                                 AttackTable::King       [sq] & (BB[White][ King ] | BB[Black][ King ]) ;

            attackers |= AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occ)] &
                    (BB[White][Bishop] | BB[Black][Bishop] | BB[White][Queen] | BB[Black][Queen]);

            attackers |= AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook, sq, occ)] &
                    (BB[White][ Rook ] | BB[Black][ Rook ] | BB[White][Queen] | BB[Black][Queen]);

            return attackers;
        }

        PreviousStateNull Move()
        {
            const auto state = PreviousStateNull(EnPassantSquare());

            Hash            = Zobrist::HashEnPassant<ZOBRIST>(Hash, EnPassantSquare());
            EnPassantTarget = BBDefault;

            CastlingRightAndColorToMove ^= ColorFlipMask;
            Hash = Zobrist::HashColorFlip<ZOBRIST>(Hash);

            return state;
        }

        void UndoMove(const PreviousStateNull& state)
        {
            if (state.EnPassant != NASQ) {
                EnPassantTarget = FromSquare(state.EnPassant);
                Hash            = Zobrist::HashEnPassant<ZOBRIST>(Hash, state.EnPassant);
            }

            CastlingRightAndColorToMove ^= ColorFlipMask;
            Hash = Zobrist::HashColorFlip<ZOBRIST>(Hash);
        }

        template<MoveType T>
        PreviousState Move(const Square from, const Square to, const Piece promotion = NAP, const size_t threadId = 0)
        {
            if (T & NNUE) Evaluation::PreMove(threadId);

            auto state = PreviousState(PieceAndColor[from], PieceAndColor[to],
                                       EnPassantSquare(), CastlingRightAndColorToMove,
                                       Hash);

            Hash            = Zobrist::HashEnPassant<T>(Hash, EnPassantSquare());
            EnPassantTarget = BBDefault;

            CastlingRightAndColorToMove ^= ColorFlipMask;
            Hash = Zobrist::HashColorFlip<T>(Hash);

            Hash = Zobrist::HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);

            const Piece pieceF = state.MovedPiece.Piece();
            const Color colorF = state.MovedPiece.Color();
            const Piece pieceT = state.CapturedPiece.Piece();
            const Color colorT = state.CapturedPiece.Color();

            using RookCastlingHandler = std::array<std::array<std::array<uint8_t, 64>, 2>, 2>;
            constexpr static RookCastlingHandler RookCastlingMask =
            [] constexpr -> RookCastlingHandler
            {
                RookCastlingHandler result = {};

                result[0] = {{}};

                result[1][0] = {};

                for (Square sq = A1; sq <= NASQ; sq = Next(sq)) {
                    if      (sq == A1) result[1][1][sq] = WhiteQCastleMask;
                    else if (sq == A8) result[1][1][sq] = BlackQCastleMask;
                    else if (sq == H1) result[1][1][sq] = WhiteKCastleMask;
                    else if (sq == H8) result[1][1][sq] = BlackKCastleMask;
                }

                return result;
            }();

            CastlingRightAndColorToMove &= ~RookCastlingMask[pieceT == Rook][
                static_cast<bool>(CastlingRightAndColorToMove & ColorCastleMask[colorT])
            ][ to ];
            CastlingRightAndColorToMove &= ~RookCastlingMask[pieceF == Rook][
                static_cast<bool>(CastlingRightAndColorToMove & ColorCastleMask[colorF])
            ][from];

            if (pieceF == Pawn) {
                if (to == state.EnPassant) {
                    const Color opposite = Opposite(colorF);

                    const auto epPawnSq = static_cast<Square>(state.EnPassant ^ 8);
                    EmptyNative(Pawn, opposite, epPawnSq);
                    Hash = Zobrist::HashPiece<T>(Hash, Pawn, opposite, epPawnSq);

                    if (T & NNUE) Evaluation::Deactivate(Pawn, opposite, epPawnSq, threadId);

                    state.EnPassantCapture = true;
                } else if (static_cast<Square>(from ^ 16) == to) {
                    const auto epSq = static_cast<Square>(to ^ 8);
                    if (T & PERFT) {
                        EnPassantTarget = FromSquare(epSq);
                        Hash            = Zobrist::HashEnPassant<T>(Hash, epSq);
                    } else {
                        if (AttackTable::Pawn[colorF][epSq] & BB[Opposite(colorF)][Pawn]) {
                            EnPassantTarget = FromSquare(epSq);
                            Hash            = Zobrist::HashEnPassant<T>(Hash, epSq);
                        }
                    }
                } else if (promotion != NAP) {
                    state.PromotedPiece = promotion;

                     EmptyNative(Pawn     , colorF, from);
                     EmptyNative(pieceT   , colorT,   to);
                    InsertNative(promotion, colorF,   to);

                    if (T & NNUE) {
                        Evaluation::Deactivate(Pawn, colorF, from, threadId);
                        Evaluation::Activate(promotion, colorF, to, threadId);

                        if (pieceT != NAP) Evaluation::Deactivate(pieceT, colorT, to, threadId);
                    }

                    Hash = Zobrist::HashPiece<T>(Hash, Pawn, colorF, from);
                    Hash = Zobrist::HashPiece<T>(Hash, pieceT, colorT, to);
                    Hash = Zobrist::HashPiece<T>(Hash, promotion, colorF, to);

                    Hash = Zobrist::HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);

                    return state;
                }
            } else if (pieceF == King && CastlingRightAndColorToMove & ColorCastleMask[colorF]) {
                CastlingRightAndColorToMove &= ~ColorCastleMask[colorF];

                if (to == C1 || to == C8 || to == G1 || to == G8) {
                    constexpr static std::array<std::array<Square, 2>, 2> RookCastleSquareStart {{
                        {H1, A1},
                        {H8, A8}
                    }};

                    constexpr static std::array<std::array<Square, 2>, 2> RookCastleSquareEnd   {{
                        {F1, D1},
                        {F8, D8}
                    }};

                    state.CastlingFrom = RookCastleSquareStart[colorF][to < from];
                    state.CastlingTo   = RookCastleSquareEnd  [colorF][to < from];

                     EmptyNative(King, colorF,               from);
                     EmptyNative(Rook, colorF, state.CastlingFrom);
                    InsertNative(King, colorF,               to  );
                    InsertNative(Rook, colorF, state.CastlingTo  );

                    if (T & NNUE) {
                        Evaluation::Transition(King, colorF,               from,               to, threadId);
                        Evaluation::Transition(Rook, colorF, state.CastlingFrom, state.CastlingTo, threadId);
                    }

                    Hash = Zobrist::HashPiece<T>(Hash, King, colorF,               from);
                    Hash = Zobrist::HashPiece<T>(Hash, Rook, colorF, state.CastlingFrom);
                    Hash = Zobrist::HashPiece<T>(Hash, King, colorF,               to  );
                    Hash = Zobrist::HashPiece<T>(Hash, Rook, colorF, state.CastlingTo  );

                    Hash = Zobrist::HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);

                    return state;
                }
            }

            MoveNative(pieceF, colorF, from, pieceT, colorT, to);

            if (T & NNUE) {
                Evaluation::Transition(pieceF, colorF, from, to, threadId);

                if (pieceT != NAP) Evaluation::Deactivate(pieceT, colorT, to, threadId);
            }

            Hash = Zobrist::HashPiece<T>(Hash, pieceF, colorF, from);
            Hash = Zobrist::HashPiece<T>(Hash, pieceT, colorT,   to);
            Hash = Zobrist::HashPiece<T>(Hash, pieceF, colorF,   to);

            Hash = Zobrist::HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);

            return state;
        }

        template<MoveType T>
        void UndoMove(const PreviousState& state, const Square from, const Square to, const size_t threadId = 0)
        {
            if (T & NNUE) Evaluation::PreUndoMove(threadId);

            CastlingRightAndColorToMove = state.CastlingRightAndColorToMove;
            if (T & ZOBRIST) Hash = state.Hash;

            if (state.EnPassant != NASQ) EnPassantTarget = FromSquare(state.EnPassant);
            else EnPassantTarget                         = BBDefault;

            if (state.PromotedPiece != NAP) {
                 EmptyNative(state.PromotedPiece, state.MovedPiece.Color(),   to);
                InsertNative(Pawn               , state.MovedPiece.Color(), from);
            } else {
                 EmptyNative(state.MovedPiece.Piece(), state.MovedPiece.Color(),   to);
                InsertNative(state.MovedPiece.Piece(), state.MovedPiece.Color(), from);
            }

            if (state.CapturedPiece.Piece() != NAP) {
                InsertNative(state.CapturedPiece.Piece(), state.CapturedPiece.Color(), to);
            } else if (state.EnPassantCapture) {
                const auto epPieceSq = static_cast<Square>(to ^ 8);
                InsertNative(Pawn, Opposite(state.MovedPiece.Color()), epPieceSq);
            } else if (state.CastlingFrom != NASQ) {
                 EmptyNative(Rook, state.MovedPiece.Color(), state.CastlingTo  );
                InsertNative(Rook, state.MovedPiece.Color(), state.CastlingFrom);
            }
        }

        void MoveNative(const Piece pF, const Color cF, const Square sqF,
                        const Piece pT, const Color cT, const Square sqT)
        {
            // Capture Section:
            Set<false>(BB[cT][pT] , sqT);

            Set<false>(ColorBB[cT], sqT);

            // MoveNative Section:
            Set<false>(BB[cF][pF], sqF);
            Set<true >(BB[cF][pF], sqT);

            Set<false>(ColorBB[cF], sqF);
            Set<true >(ColorBB[cF], sqT);

            UpdateNACBB();

            PieceAndColor[sqT] = PieceAndColor[sqF];
            PieceAndColor[sqF] = PieceColor(NAP, NAC);
        }

        void EmptyNative(const Piece p, const Color c, const Square sq)
        {
            Set<false>(BB[c][p]  , sq);

            Set<false>(ColorBB[c], sq);

            UpdateNACBB();

            PieceAndColor[sq] = PieceColor(NAP, NAC);
        }

        void InsertNative(const Piece p, const Color c, const Square sq)
        {
            Set<true>(BB[c][p]  , sq);

            Set<true>(ColorBB[c], sq);

            UpdateNACBB();

            PieceAndColor[sq] = PieceColor(p, c);
        }

        void UpdateNACBB()
        {
            ColorBB[NAC] = ~(ColorBB[White] | ColorBB[Black]);
        }

    };

} // StockDory

#endif //STOCKDORY_BOARD_H
