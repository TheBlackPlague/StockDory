//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_BOARD_H
#define STOCKDORY_BOARD_H

#include <array>
#include <string>
#include <cassert>

#include "Type/BitBoard.h"
#include "Type/Piece.h"
#include "Type/Color.h"
#include "Type/PieceColor.h"
#include "Type/PinBitBoard.h"
#include "Type/CheckBitBoard.h"
#include "Type/PreviousState.h"

#include "Template/MoveType.h"

#include "Move/AttackTable.h"
#include "Move/UtilityTable.h"
#include "Move/BlackMagicFactory.h"

#include "../External/strutil.h"

#include "Util.h"

namespace StockDory
{

    class Board
    {

        private:
            std::array<std::array<BitBoard, 7>, 3> BB;

            std::array<PieceColor, 64> PieceAndColor;

            std::array<BitBoard, 3> ColorBB;

            // [COLOR TO MOVE] [WHITE KING CASTLE] [WHITE QUEEN CASTLE] [BLACK KING CASTLE] [BLACK QUEEN CASTLE]
            // [    4 BITS   ] [      1 BIT      ] [       1 BIT      ] [      1 BIT      ] [       1 BIT      ]
            uint8_t CastlingRightAndColorToMove;

            BitBoard EnPassantTarget;

            constexpr static uint8_t  ColorToMoveMask = 0xF0;
            constexpr static uint8_t WhiteKCastleMask = 0x08;
            constexpr static uint8_t WhiteQCastleMask = 0x04;
            constexpr static uint8_t BlackKCastleMask = 0x02;
            constexpr static uint8_t BlackQCastleMask = 0x01;

            constexpr static uint8_t ColorFlipMask = 0x10;

            constexpr static std::array<uint8_t, 2> ColorCastleMask {
                WhiteKCastleMask | WhiteQCastleMask,
                BlackKCastleMask | BlackQCastleMask
            };

            constexpr static std::array<std::array<Square, 2>, 2> CastleRookSquareStart {{
                {Square::H1, Square::A1},
                {Square::H8, Square::A8}
            }};

            constexpr static std::array<std::array<Square, 2>, 2> CastleRookSquareEnd {{
                {Square::F1, Square::D1},
                {Square::F8, Square::D8}
            }};

            constexpr inline void UpdateNACBB()
            {
                ColorBB[Color::NAC] = ~(ColorBB[Color::White] | ColorBB[Color::Black]);
            }

        public:
            Board() : Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
            explicit Board(const std::string& fen)
            {
                PieceColor none = PieceColor(Piece::NAP, Color::NAC);
                std::fill(std::begin(PieceAndColor), std::end(PieceAndColor), none);

                for (uint8_t i = 0; i < 3; i++)
                    std::fill(std::begin(BB[i]), std::end(BB[i]), BBDefault);

                std::vector<std::string> splitFen = strutil::split(fen, " ");

                assert(splitFen.size() == 6);

                std::vector<std::string> splitPosition = strutil::split(splitFen[0], "/");
                std::reverse(splitPosition.begin(), splitPosition.end());

                assert(splitPosition.size() == 8);

                for (uint8_t v = 0; v < 8; v++) {
                    std::string& rankStr = splitPosition[v];
                    uint8_t h = 0;
                    for (char p : rankStr) {
                        if (isdigit(p)) {
                            h += static_cast<uint8_t>(p - 48);
                            continue;
                        }

                        Color color = Color::Black;

                        if (isupper(p)) color = Color::White;

                        Piece piece = Piece::NAP;
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
                        }

                        uint8_t idx = v * 8 + h;
                        Set<true>(BB[color][piece], static_cast<Square>(idx));
                        PieceAndColor[idx] = PieceColor(piece, color);

                        h++;
                    }
                }

                CastlingRightAndColorToMove = splitFen[1][0] == 'w' ? Color::White << 4 : Color::Black << 4;

                std::string& castlingData = splitFen[2];
                CastlingRightAndColorToMove |= (castlingData.find('K') != std::string::npos ? 0x8 : 0x0);
                CastlingRightAndColorToMove |= (castlingData.find('Q') != std::string::npos ? 0x4 : 0x0);
                CastlingRightAndColorToMove |= (castlingData.find('k') != std::string::npos ? 0x2 : 0x0);
                CastlingRightAndColorToMove |= (castlingData.find('q') != std::string::npos ? 0x1 : 0x0);

                EnPassantTarget = BBDefault;
                std::string& epData = splitFen[3];
                if (epData.length() == 2) {
                    EnPassantTarget = FromSquare(Util::StringToSquare(epData));
                }

                ColorBB[Color::White] = BBDefault;
                ColorBB[Color::Black] = BBDefault;
                for (Piece p = Piece::Pawn; p != Piece::NAP; p = Next(p)) {
                    ColorBB[Color::White] |= BB[Color::White][p];
                    ColorBB[Color::Black] |= BB[Color::Black][p];
                }
                ColorBB[Color::NAC] = ~(ColorBB[Color::White] | ColorBB[Color::Black]);
            }
#pragma clang diagnostic pop

            constexpr inline PieceColor operator [](const Square sq) const
            {
                return PieceAndColor[sq];
            }

            constexpr inline BitBoard operator [](const Color c) const
            {
                return ColorBB[c];
            }

            template<Color Color>
            [[nodiscard]]
            constexpr inline BitBoard PieceBoard(const Piece p) const
            {
                assert(p     != Piece::NAP);
                assert(Color != Color::NAC);

                return BB[Color][p];
            }

            [[nodiscard]]
            constexpr inline BitBoard PieceBoard(const Piece p, const Color c) const
            {
                assert(p != Piece::NAP);
                assert(c != Color::NAC);

                return BB[c][p];
            }

            [[nodiscard]]
            constexpr inline Color ColorToMove() const
            {
                return static_cast<Color>(CastlingRightAndColorToMove & ColorToMoveMask);
            }

            template<Color Color>
            [[nodiscard]]
            constexpr inline bool CastlingRight() const
            {
                if (Color == White) return CastlingRightAndColorToMove & (WhiteKCastleMask | WhiteQCastleMask);
                if (Color == Black) return CastlingRightAndColorToMove & (BlackKCastleMask | BlackQCastleMask);

                throw std::invalid_argument("Invalid color");
            }

            template<Color Color>
            [[nodiscard]]
            constexpr inline bool CastlingRightK() const
            {
                if (Color == White) return CastlingRightAndColorToMove & WhiteKCastleMask;
                if (Color == Black) return CastlingRightAndColorToMove & BlackKCastleMask;

                throw std::invalid_argument("Invalid color");
            }

            template<Color Color>
            [[nodiscard]]
            constexpr inline bool CastlingRightQ() const
            {
                if (Color == White) return CastlingRightAndColorToMove & WhiteQCastleMask;
                if (Color == Black) return CastlingRightAndColorToMove & BlackQCastleMask;

                throw std::invalid_argument("Invalid color");
            }

            [[nodiscard]]
            constexpr inline BitBoard EnPassant() const
            {
                return EnPassantTarget;
            }

            [[nodiscard]]
            constexpr inline Square EnPassantSquare() const
            {
                return ToSquare(EnPassantTarget);
            }

            template<Color By>
            [[nodiscard]]
            constexpr inline CheckBitBoard Check() const
            {
                uint8_t       count = 0;
                CheckBitBoard check = CheckBitBoard();

                const Square sq = ToSquare(BB[Opposite(By)][King]);

                // Check if the square is under attack by opponent knights or pawns.
                const BitBoard pawnCheck   = AttackTable::Pawn[Opposite(By)][sq] & BB[By][Pawn  ];
                const BitBoard knightCheck = AttackTable::Knight               [sq] & BB[By][Knight];

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
                BitBoard occupied = ~ColorBB[Color::NAC];

                // Check if the square is under attack by opponent bishops or queens (diagonally).
                const BitBoard diagonalCheck =
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Bishop, sq, occupied)] &
                        (queen | BB[By][Bishop]);

                // Check if the square is under attack by opponent rooks or queens (straight).
                const BitBoard straightCheck =
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Rook  , sq, occupied)] &
                        (queen | BB[By][Rook  ]);

                // For sliding attacks, we must add the square of the attack's origin and all the squares to us from the
                // attack:
                if (diagonalCheck) {
                    const Square diagonalCheckSq = ToSquare(diagonalCheck);
                    check.Check |= UtilityTable::Between[sq][diagonalCheckSq] | FromSquare(diagonalCheckSq);
                }

                if (straightCheck) {
                    const Square straightCheckSq = ToSquare(straightCheck);
                    check.Check |= UtilityTable::Between[sq][straightCheckSq] | FromSquare(straightCheckSq);
                }

                count += static_cast<bool>(diagonalCheck);
                count += static_cast<bool>(straightCheck);

                // In the case where there is more than one check, we must increment the count once more, as it's a
                // double check.
                if (Count(straightCheck) > 1) count++;

                if (check.Check == BBDefault) check.Check = BBFilled;

                check.DoubleCheck = count > 1;
                return check;
            }

            template<Color We, Color By>
            [[nodiscard]]
            constexpr inline PinBitBoard Pin() const
            {
                PinBitBoard pin = PinBitBoard();

                const Square sq = ToSquare(BB[We][King]);

                // All the occupied squares:
                // In this case, we want to let the pins pass through our pieces, since our pieces can move on the pins.
                const BitBoard occupied = ColorBB[By];

                // For queen, we can merge with checks for bishop and rook.
                const BitBoard queen = BB[By][Queen];

                // Check if the square is under attack by opponent bishops or queens (diagonally).
                const BitBoard diagonalCheck =
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Bishop, sq, occupied)] &
                        (queen | BB[By][Bishop]);

                // Check if the square is under attack by opponent rooks or queens (straight).
                const BitBoard straightCheck =
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Piece::Rook  , sq, occupied)] &
                        (queen | BB[By][Rook  ]);

                // Iterate through the attacks and check if the attack is a diagonally pinning one.
                BitBoardIterator iterator (diagonalCheck);
                for (Square attSq = iterator.Value(); attSq != Square::NASQ; attSq = iterator.Value()) {
                    const BitBoard possiblePin = UtilityTable::Between[sq][attSq] | FromSquare(attSq);
                    if (Count(possiblePin & ColorBB[We]) == 1) pin.Diagonal |= possiblePin;
                }

                // Iterate through the attacks and check if the attack is a straight pinning one.
                iterator = BitBoardIterator(straightCheck);
                for (Square attSq = iterator.Value(); attSq != Square::NASQ; attSq = iterator.Value()) {
                    const BitBoard possiblePin = UtilityTable::Between[sq][attSq] | FromSquare(attSq);
                    if (Count(possiblePin & ColorBB[We]) == 1) pin.Straight |= possiblePin;
                }

                return pin;
            }

            template<MoveType T>
            constexpr inline PreviousState Move(const Square from, const Square to, const Piece promotion = NAP)
            {
                PreviousState state = PreviousState(PieceAndColor[from], PieceAndColor[to],
                                                    EnPassantSquare(), CastlingRightAndColorToMove);

                const Piece pieceF = state.MovedPiece   .Piece();
                const Color colorF = state.MovedPiece   .Color();
                const Piece pieceT = state.CapturedPiece.Piece();
                const Color colorT = state.CapturedPiece.Color();

                if (pieceT == Rook && (CastlingRightAndColorToMove & ColorCastleMask[colorT])) {
                    if (to == A1) CastlingRightAndColorToMove &= ~WhiteQCastleMask;
                    if (to == A8) CastlingRightAndColorToMove &= ~BlackQCastleMask;
                    if (to == H1) CastlingRightAndColorToMove &= ~WhiteKCastleMask;
                    if (to == H8) CastlingRightAndColorToMove &= ~BlackKCastleMask;
                }

                if (pieceF == Pawn) {
                    if (to == state.EnPassant) {
                        EmptyNative<T>(Pawn, Opposite(colorF), static_cast<Square>(state.EnPassant ^ 8));
                        state.EnPassantCapture = true;
                        EnPassantTarget = BBDefault;
                    } else if (std::abs(from - to) == 16) {
                        auto epSq = static_cast<Square>((from + to) >> 1);
                        if (AttackTable::Pawn[colorF][epSq] & BB[Opposite(colorF)][Pawn])
                             EnPassantTarget = FromSquare(static_cast<Square>(to ^ 8));
                        else EnPassantTarget = BBDefault;
                    } else if (promotion != NAP) {
                        EmptyNative <T>(Pawn     , colorF, from);
                        EmptyNative <T>(pieceT   , colorT, to  );
                        InsertNative<T>(promotion, colorF, to  );

                        state.PromotedPiece = promotion;

                        EnPassantTarget = BBDefault;
                        CastlingRightAndColorToMove ^= ColorFlipMask;
                        return state;
                    } else EnPassantTarget = BBDefault;
                } else {
                    EnPassantTarget = BBDefault;

                    if ((CastlingRightAndColorToMove & ColorCastleMask[colorF])) {
                        if        (pieceF == Rook) {
                            if (from == A1) CastlingRightAndColorToMove &= ~WhiteQCastleMask;
                            if (from == A8) CastlingRightAndColorToMove &= ~BlackQCastleMask;
                            if (from == H1) CastlingRightAndColorToMove &= ~WhiteKCastleMask;
                            if (from == H8) CastlingRightAndColorToMove &= ~BlackKCastleMask;
                        } else if (pieceF == King) {
                            CastlingRightAndColorToMove &= ~ColorCastleMask[colorF];

                            if (std::abs(from - to) == 2) {
                                state.CastlingFrom = CastleRookSquareStart[colorF][to < from];
                                state.CastlingTo   = CastleRookSquareEnd  [colorF][to < from];

                                EmptyNative <T>(King, colorF, from              );
                                EmptyNative <T>(Rook, colorF, state.CastlingFrom);
                                InsertNative<T>(King, colorF, to                );
                                InsertNative<T>(Rook, colorF, state.CastlingTo  );

                                CastlingRightAndColorToMove ^= ColorFlipMask;
                                return state;
                            }
                        }
                    }
                }

                MoveNative<T>(pieceF, colorF, from, pieceT, colorT, to);
                CastlingRightAndColorToMove ^= ColorFlipMask;

                return state;
            }

            template<MoveType T>
            constexpr inline void UndoMove(const PreviousState state, const Square from, const Square to)
            {
                CastlingRightAndColorToMove = state.CastlingRightAndColorToMove;\

                if (state.EnPassant != NASQ) EnPassantTarget = FromSquare(state.EnPassant);
                else                         EnPassantTarget = BBDefault;

                if (state.PromotedPiece         != NAP) {
                    EmptyNative <T>(state.PromotedPiece, state.MovedPiece.Color(), to  );
                    InsertNative<T>(Pawn               , state.MovedPiece.Color(), from);
                } else {
                    EmptyNative <T>(state.MovedPiece.Piece(), state.MovedPiece.Color(), to  );
                    InsertNative<T>(state.MovedPiece.Piece(), state.MovedPiece.Color(), from);
                }

                if (state.CapturedPiece.Piece() != NAP) {
                    InsertNative<T>(state.CapturedPiece.Piece(), state.CapturedPiece.Color(), to);
                } else if (state.EnPassantCapture) {
                    auto epPieceSq = static_cast<Square>(to ^ 8);
                    InsertNative<T>(Pawn, Opposite(state.MovedPiece.Color()), epPieceSq);
                } else if (state.CastlingFrom != NASQ) {
                    EmptyNative <T>(Rook, state.MovedPiece.Color(), state.CastlingTo  );
                    InsertNative<T>(Rook, state.MovedPiece.Color(), state.CastlingFrom);
                }
            }

            template<MoveType T>
            constexpr inline void MoveNative(const Piece pF, const Color cF, const Square sqF,
                                             const Piece pT, const Color cT, const Square sqT)
            {
                // Capture Section:
                Set<false>(BB[cT][pT], sqT);

                Set<false>(ColorBB[cT], sqT);

                // MoveNative Section:
                Set<false>(BB[cF][pF], sqF);
                Set<true >(BB[cF][pF], sqT);

                Set<false>(ColorBB[cF], sqF);
                Set<true >(ColorBB[cF], sqT);

                UpdateNACBB();

                PieceAndColor[sqT] = PieceAndColor[sqF];
                PieceAndColor[sqF] = PieceColor(Piece::NAP, Color::NAC);
            }

            template<MoveType T>
            constexpr inline void EmptyNative(const Piece p, const Color c, const Square sq)
            {
                Set<false>(BB[c][p], sq);

                Set<false>(ColorBB[c], sq);

                UpdateNACBB();

                PieceAndColor[sq] = PieceColor(Piece::NAP, Color::NAC);
            }

            template<MoveType T>
            constexpr inline void InsertNative(const Piece p, const Color c, const Square sq)
            {
                Set<true>(BB[c][p], sq);

                Set<true>(ColorBB[c], sq);

                UpdateNACBB();

                PieceAndColor[sq] = PieceColor(p, c);
            }

    };

} // StockDory

#endif //STOCKDORY_BOARD_H
