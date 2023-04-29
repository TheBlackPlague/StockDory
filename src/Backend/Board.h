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
#include "Type/Zobrist.h"

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
            std::array<std::array<BitBoard, 7>, 3> BB {};

            std::array<PieceColor, 64> PieceAndColor {};

            std::array<BitBoard, 3> ColorBB {};

            // [COLOR TO MOVE] [WHITE KING CASTLE] [WHITE QUEEN CASTLE] [BLACK KING CASTLE] [BLACK QUEEN CASTLE]
            // [    4 BITS   ] [      1 BIT      ] [       1 BIT      ] [      1 BIT      ] [       1 BIT      ]
            uint8_t CastlingRightAndColorToMove;

            BitBoard EnPassantTarget;

            ZobristHash Hash;

            constexpr static uint8_t  ColorToMoveMask = 0xF0;
            constexpr static uint8_t     CastlingMask = 0x0F;
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

        public:
            constexpr Board() : Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}

            constexpr explicit Board(const std::string& fen)
            {
                PieceColor none = PieceColor(NAP, NAC);
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
                        }

                        uint8_t idx = v * 8 + h;
                        Set<true>(BB[color][piece], static_cast<Square>(idx));
                        PieceAndColor[idx] = PieceColor(piece, color);
                        Hash = HashPiece<ZOBRIST>(Hash, piece, color, static_cast<Square>(idx));

                        h++;
                    }
                }

                if (splitFen[1][0] == 'w') {
                    CastlingRightAndColorToMove = White << 4;
                    Hash = HashColorFlip<ZOBRIST>(Hash);
                } else {
                    CastlingRightAndColorToMove = Black << 4;
                }

                std::string& castlingData = splitFen[2];
                CastlingRightAndColorToMove |= (castlingData.find('K') != std::string::npos ? 0x8 : 0x0);
                CastlingRightAndColorToMove |= (castlingData.find('Q') != std::string::npos ? 0x4 : 0x0);
                CastlingRightAndColorToMove |= (castlingData.find('k') != std::string::npos ? 0x2 : 0x0);
                CastlingRightAndColorToMove |= (castlingData.find('q') != std::string::npos ? 0x1 : 0x0);

                Hash = HashCastling<ZOBRIST>(Hash, CastlingRightAndColorToMove & CastlingMask);

                EnPassantTarget = BBDefault;
                std::string& epData = splitFen[3];
                if (epData.length() == 2) {
                    Square epSq = Util::StringToSquare(epData);
                    EnPassantTarget = FromSquare(epSq);
                    Hash = HashEnPassant<ZOBRIST>(Hash, epSq);
                }

                ColorBB[Color::White] = BBDefault;
                ColorBB[Color::Black] = BBDefault;
                for (Piece p = Pawn; p != NAP; p = Next(p)) {
                    ColorBB[White] |= BB[White][p];
                    ColorBB[Black] |= BB[Black][p];
                }
                ColorBB[NAC] = ~(ColorBB[White] | ColorBB[Black]);
            }

            [[nodiscard]]
            constexpr inline ZobristHash GetHash() const
            {
                return Hash;
            }

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
                assert(p     != NAP);
                assert(Color != NAC);

                return BB[Color][p];
            }

            [[nodiscard]]
            constexpr inline BitBoard PieceBoard(const Piece p, const Color c) const
            {
                assert(p != NAP);
                assert(c != NAC);

                return BB[c][p];
            }

            [[nodiscard]]
            constexpr inline Color ColorToMove() const
            {
                return static_cast<Color>(CastlingRightAndColorToMove & ColorToMoveMask);
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
                BitBoard occupied = ~ColorBB[NAC];

                // Check if the square is under attack by opponent bishops or queens (diagonally).
                const BitBoard diagonalCheck =
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occupied)] &
                        (queen | BB[By][Bishop]);

                // Check if the square is under attack by opponent rooks or queens (straight).
                const BitBoard straightCheck =
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook  , sq, occupied)] &
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
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Bishop, sq, occupied)] &
                        (queen | BB[By][Bishop]);

                // Check if the square is under attack by opponent rooks or queens (straight).
                const BitBoard straightCheck =
                        AttackTable::Sliding[BlackMagicFactory::MagicIndex(Rook  , sq, occupied)] &
                        (queen | BB[By][Rook  ]);

                // Iterate through the attacks and check if the attack is a diagonally pinning one.
                BitBoardIterator iterator (diagonalCheck);
                for (Square attSq = iterator.Value(); attSq != NASQ; attSq = iterator.Value()) {
                    const BitBoard possiblePin = UtilityTable::Between[sq][attSq] | FromSquare(attSq);
                    if (Count(possiblePin & ColorBB[We]) == 1) pin.Diagonal |= possiblePin;
                }

                // Iterate through the attacks and check if the attack is a straight pinning one.
                iterator = BitBoardIterator(straightCheck);
                for (Square attSq = iterator.Value(); attSq != NASQ; attSq = iterator.Value()) {
                    const BitBoard possiblePin = UtilityTable::Between[sq][attSq] | FromSquare(attSq);
                    if (Count(possiblePin & ColorBB[We]) == 1) pin.Straight |= possiblePin;
                }

                return pin;
            }

            template<MoveType T>
            constexpr inline PreviousState Move(const Square from, const Square to, const Piece promotion = NAP)
            {
                PreviousState state = PreviousState(PieceAndColor[from], PieceAndColor[to],
                                                    EnPassantSquare(), CastlingRightAndColorToMove,
                                                    Hash);

                Hash = HashEnPassant<T>(Hash, EnPassantSquare());
                EnPassantTarget = BBDefault;

                CastlingRightAndColorToMove ^= ColorFlipMask;
                Hash = HashColorFlip<T>(Hash);

                const Piece pieceF = state.MovedPiece   .Piece();
                const Color colorF = state.MovedPiece   .Color();
                const Piece pieceT = state.CapturedPiece.Piece();
                const Color colorT = state.CapturedPiece.Color();

                if (pieceT == Rook && (CastlingRightAndColorToMove & ColorCastleMask[colorT])) {
                    if (to == A1) {
                        CastlingRightAndColorToMove &= ~WhiteQCastleMask;
                        Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                    } else if (to == A8) {
                        CastlingRightAndColorToMove &= ~BlackQCastleMask;
                        Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                    } else if (to == H1) {
                        CastlingRightAndColorToMove &= ~WhiteKCastleMask;
                        Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                    } else if (to == H8) {
                        CastlingRightAndColorToMove &= ~BlackKCastleMask;
                        Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                    }
                }

                if (pieceF == Pawn) {
                    if (to == state.EnPassant) {
                        const auto epPawnSq = static_cast<Square>(state.EnPassant ^ 8);
                        EmptyNative<T>(           Pawn, Opposite(colorF), epPawnSq);
                        Hash = HashPiece<T>(Hash, Pawn, Opposite(colorF), epPawnSq);
                        state.EnPassantCapture = true;
                    } else if (static_cast<Square>(from ^ 16) == to) {
                        const auto epSq = static_cast<Square>(to ^ 8);
                        if (AttackTable::Pawn[colorF][epSq] & BB[Opposite(colorF)][Pawn]) {
                            EnPassantTarget = FromSquare(epSq);
                            Hash = HashEnPassant<T>(Hash, epSq);
                        }
                    } else if (promotion != NAP) {
                        EmptyNative <T>(Pawn     , colorF, from);
                        EmptyNative <T>(pieceT   , colorT, to  );
                        InsertNative<T>(promotion, colorF, to  );

                        Hash = HashPiece<T>(Hash, Pawn     , colorF, from);
                        Hash = HashPiece<T>(Hash, pieceT   , colorF, to  );
                        Hash = HashPiece<T>(Hash, promotion, colorF, to  );

                        state.PromotedPiece = promotion;
                        return state;
                    }
                } else if ((CastlingRightAndColorToMove & ColorCastleMask[colorF])) {
                    if        (pieceF == Rook) {
                        if (from == A1) {
                            CastlingRightAndColorToMove &= ~WhiteQCastleMask;
                            Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                        } else if (from == A8) {
                            CastlingRightAndColorToMove &= ~BlackQCastleMask;
                            Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                        } else if (from == H1) {
                            CastlingRightAndColorToMove &= ~WhiteKCastleMask;
                            Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                        } else if (from == H8) {
                            CastlingRightAndColorToMove &= ~BlackKCastleMask;
                            Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);
                        }
                    } else if (pieceF == King) {
                        CastlingRightAndColorToMove &= ~ColorCastleMask[colorF];
                        Hash = HashCastling<T>(Hash, CastlingRightAndColorToMove & CastlingMask);

                        if (to == C1 || to == C8 || to == G1 || to == G8) {
                            state.CastlingFrom = CastleRookSquareStart[colorF][to < from];
                            state.CastlingTo   = CastleRookSquareEnd  [colorF][to < from];

                            EmptyNative <T>(King, colorF, from              );
                            EmptyNative <T>(Rook, colorF, state.CastlingFrom);
                            InsertNative<T>(King, colorF, to                );
                            InsertNative<T>(Rook, colorF, state.CastlingTo  );

                            Hash = HashPiece<T>(Hash, King, colorF, from              );
                            Hash = HashPiece<T>(Hash, Rook, colorF, state.CastlingFrom);
                            Hash = HashPiece<T>(Hash, King, colorF, to                );
                            Hash = HashPiece<T>(Hash, Rook, colorF, state.CastlingTo  );

                            return state;
                        }
                    }
                }

                MoveNative<T>(pieceF, colorF, from, pieceT, colorT, to);
                Hash = HashPiece<T>(Hash, pieceF, colorF, from);
                Hash = HashPiece<T>(Hash, pieceT, colorT, to  );
                Hash = HashPiece<T>(Hash, pieceF, colorF, to  );

                return state;
            }

            template<MoveType T>
            constexpr inline void UndoMove(const PreviousState& state, const Square from, const Square to)
            {
                CastlingRightAndColorToMove = state.CastlingRightAndColorToMove;
                if (T & ZOBRIST) Hash = state.Hash;

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
                PieceAndColor[sqF] = PieceColor(NAP, NAC);
            }

            template<MoveType T>
            constexpr inline void EmptyNative(const Piece p, const Color c, const Square sq)
            {
                Set<false>(BB[c][p], sq);

                Set<false>(ColorBB[c], sq);

                UpdateNACBB();

                PieceAndColor[sq] = PieceColor(NAP, NAC);
            }

            template<MoveType T>
            constexpr inline void InsertNative(const Piece p, const Color c, const Square sq)
            {
                Set<true>(BB[c][p], sq);

                Set<true>(ColorBB[c], sq);

                UpdateNACBB();

                PieceAndColor[sq] = PieceColor(p, c);
            }

            constexpr inline void UpdateNACBB()
            {
                ColorBB[Color::NAC] = ~(ColorBB[Color::White] | ColorBB[Color::Black]);
            }

    };

} // StockDory

#endif //STOCKDORY_BOARD_H
