//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_BITBOARD_H
#define STOCKDORY_BITBOARD_H

#include <bit>
#include <iostream>

#include "Base.h"
#include "PieceType.h"
#include "Square.h"

namespace StockDory
{

    using BitBoard = u64;

    constexpr u08 Count(const BitBoard bb) { return std::popcount(bb); }

    template<bool Activate>
    constexpr void Set(BitBoard& bb, const Square sq)
    {
        if (Activate) bb |=   1ULL << sq ;
        else          bb &= ~(1ULL << sq);
    }

    constexpr bool Get(const BitBoard bb, const Square sq) { return bb >> sq & 1ULL; }

    constexpr BitBoard AsBitBoard(const Square sq) { return 1ULL << sq; }

    constexpr Square AsSquare(const BitBoard bb) { return static_cast<Square>(std::countr_zero(bb)); }

    class BitBoardIterator
    {

        BitBoard Internal;

        public:
        constexpr BitBoardIterator(const BitBoard bb) : Internal(bb) {}

        constexpr Square operator *() const { return AsSquare(Internal); }

        constexpr BitBoardIterator& operator ++() { Internal &= Internal - 1; return *this; }

        constexpr bool operator !=(const BitBoardIterator& other) const { return Internal != other.Internal; }

        constexpr BitBoardIterator begin() const { return *this; }
        constexpr BitBoardIterator   end() const { return *this; }

    };

    struct   PinBitBoard { BitBoard StraightMask = 0, DiagonalMask = 0; };
    struct CheckBitBoard { BitBoard Mask = 0; bool Double = false; };

    namespace Ray
    {

        enum File : u08 { FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH };
        enum Rank : u08 { Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8 };

        constexpr Array<BitBoard, 8> File {
            0x00000000000000FF, 0x000000000000FF00, 0x0000000000FF0000, 0x00000000FF000000,
            0x000000FF00000000, 0x0000FF0000000000, 0x00FF000000000000, 0xFF00000000000000
        };

        constexpr Array<BitBoard, 8> Rank {
            0x0101010101010101, 0x0202020202020202, 0x0404040404040404, 0x0808080808080808,
            0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080
        };

    } // StockDory::Ray

    namespace BlackMagic
    {

        constexpr Array<u08, 2> PieceK { 9, 12 };

        struct MagicConstant { BitBoard Magic; u32 Base; };

        constexpr Array<MagicConstant, 64> Bishop {{
            { 0xA7020080601803D8, 60984 }, { 0x13802040400801F1, 66046 },
            { 0x0A0080181001F60C, 32910 }, { 0x1840802004238008, 16369 },
            { 0xC03FE00100000000, 42115 }, { 0x24C00BFFFF400000,   835 },
            { 0x0808101F40007F04, 18910 }, { 0x100808201EC00080, 25911 },
            { 0xFFA2FEFFBFEFB7FF, 63301 }, { 0x083E3EE040080801, 16063 },
            { 0xC0800080181001F8, 17481 }, { 0x0440007FE0031000, 59361 },
            { 0x2010007FFC000000, 18735 }, { 0x1079FFE000FF8000, 61249 },
            { 0x3C0708101F400080, 68938 }, { 0x080614080FA00040, 61791 },
            { 0x7FFE7FFF817FCFF9, 21893 }, { 0x7FFEBFFFA01027FD, 62068 },
            { 0x53018080C00F4001, 19829 }, { 0x407E0001000FFB8A, 26091 },
            { 0x201FE000FFF80010, 15815 }, { 0xFFDFEFFFDE39FFEF, 16419 },
            { 0xCC8808000FBF8002, 59777 }, { 0x7FF7FBFFF8203FFF, 16288 },
            { 0x8800013E8300C030, 33235 }, { 0x0420009701806018, 15459 },
            { 0x7FFEFF7F7F01F7FD, 15863 }, { 0x8700303010C0C006, 75555 },
            { 0xC800181810606000, 79445 }, { 0x20002038001C8010, 15917 },
            { 0x087FF038000FC001,  8512 }, { 0x00080C0C00083007, 73069 },
            { 0x00000080FC82C040, 16078 }, { 0x000000407E416020, 19168 },
            { 0x00600203F8008020, 11056 }, { 0xD003FEFE04404080, 62544 },
            { 0xA00020C018003088, 80477 }, { 0x7FBFFE700BFFE800, 75049 },
            { 0x107FF00FE4000F90, 32947 }, { 0x7F8FFFCFF1D007F8, 59172 },
            { 0x0000004100F88080, 55845 }, { 0x00000020807C4040, 61806 },
            { 0x00000041018700C0, 73601 }, { 0x0010000080FC4080, 15546 },
            { 0x1000003C80180030, 45243 }, { 0xC10000DF80280050, 20333 },
            { 0xFFFFFFBFEFF80FDC, 33402 }, { 0x000000101003F812, 25917 },
            { 0x0800001F40808200, 32875 }, { 0x084000101F3FD208,  4639 },
            { 0x080000000F808081, 17077 }, { 0x0004000008003F80, 62324 },
            { 0x08000001001FE040, 18159 }, { 0x72DD000040900A00, 61436 },
            { 0xFFFFFEFFBFEFF81D, 57073 }, { 0xCD8000200FEBF209, 61025 },
            { 0x100000101EC10082, 81259 }, { 0x7FBAFFFFEFE0C02F, 64083 },
            { 0x7F83FFFFFFF07F7F, 56114 }, { 0xFFF1FFFFFFF7FFC1, 57058 },
            { 0x0878040000FFE01F, 58912 }, { 0x945E388000801012, 22194 },
            { 0x0840800080200FDA, 70880 }, { 0x100000C05F582008, 11140 }
        }};

        constexpr Array<MagicConstant, 64> Rook   {{
            { 0x80280013FF84FFFF, 10890 }, { 0x5FFBFEFDFEF67FFF, 50579 },
            { 0xFFEFFAFFEFFDFFFF, 62020 }, { 0x003000900300008A, 67322 },
            { 0x0050028010500023, 80251 }, { 0x0020012120A00020, 58503 },
            { 0x0030006000C00030, 51175 }, { 0x0058005806B00002, 83130 },
            { 0x7FBFF7FBFBEAFFFC, 50430 }, { 0x0000140081050002, 21613 },
            { 0x0000180043800048, 72625 }, { 0x7FFFE800021FFFB8, 80755 },
            { 0xFFFFCFFE7FCFFFAF, 69753 }, { 0x00001800C0180060, 26973 },
            { 0x4F8018005FD00018, 84972 }, { 0x0000180030620018, 31958 },
            { 0x00300018010C0003, 69272 }, { 0x0003000C0085FFFF, 48372 },
            { 0xFFFDFFF7FBFEFFF7, 65477 }, { 0x7FC1FFDFFC001FFF, 43972 },
            { 0xFFFEFFDFFDFFDFFF, 57154 }, { 0x7C108007BEFFF81F, 53521 },
            { 0x20408007BFE00810, 30534 }, { 0x0400800558604100, 16548 },
            { 0x0040200010080008, 46407 }, { 0x0010020008040004, 11841 },
            { 0xFFFDFEFFF7FBFFF7, 21112 }, { 0xFEBF7DFFF8FEFFF9, 44214 },
            { 0xC00000FFE001FFE0, 57925 }, { 0x4AF01F00078007C3, 29574 },
            { 0xBFFBFAFFFB683F7F, 17309 }, { 0x0807F67FFA102040, 40143 },
            { 0x200008E800300030, 64659 }, { 0x0000008780180018, 70469 },
            { 0x0000010300180018, 62917 }, { 0x4000008180180018, 60997 },
            { 0x008080310005FFFA, 18554 }, { 0x4000188100060006, 14385 },
            { 0xFFFFFF7FFFBFBFFF,     0 }, { 0x0000802000200040, 38091 },
            { 0x20000202EC002800, 25122 }, { 0xFFFFF9FF7CFFF3FF, 60083 },
            { 0x000000404B801800, 72209 }, { 0x2000002FE03FD000, 67875 },
            { 0xFFFFFF6FFE7FCFFD, 56290 }, { 0xBFF7EFFFBFC00FFF, 43807 },
            { 0x000000100800A804, 73365 }, { 0x6054000A58005805, 76398 },
            { 0x0829000101150028, 20024 }, { 0x00000085008A0014,  9513 },
            { 0x8000002B00408028, 24324 }, { 0x4000002040790028, 22996 },
            { 0x7800002010288028, 23213 }, { 0x0000001800E08018, 56002 },
            { 0xA3A80003F3A40048, 22809 }, { 0x2003D80000500028, 44545 },
            { 0xFFFFF37EEFEFDFBE, 36072 }, { 0x40000280090013C1,  4750 },
            { 0xBF7FFEFFBFFAF71F,  6014 }, { 0xFFFDFFFF777B7D6E, 36054 },
            { 0x48300007E8080C02, 78538 }, { 0xAFE0000FFF780402, 28745 },
            { 0xEE73FFFBFFBB77FE,  8555 }, { 0x0002000308482882,  1009 }
        }};

        template<PieceType Type>
        constexpr BitBoard OccupationMask(const Square sq)
        {
            if (Type == PieceType::Bishop) {
                const u08 file = sq % 8;
                const u08 rank = sq / 8;

                BitBoard ray {};

                for (u08 f = 0; f < 8; f++)
                for (u08 r = 0; r < 8; r++) {
                    const u08 dF = f - file < 0 ? -(f - file) : f - file;
                    const u08 dR = r - rank < 0 ? -(r - rank) : r - rank;

                    if (dF == dR && dR != 0) ray |= AsBitBoard(static_cast<Square>(r * 8 + f));
                }

                return ray & ~(Ray::File[Ray::FileA] | Ray::File[Ray::FileH] |
                               Ray::Rank[Ray::Rank1] | Ray::Rank[Ray::Rank8] );
            }

            if (Type == PieceType::Rook) {
                const BitBoard file = Ray::File[sq % 8] & ~(Ray::Rank[Ray::Rank1] | Ray::Rank[Ray::Rank8]);
                const BitBoard rank = Ray::Rank[sq / 8] & ~(Ray::File[Ray::FileA] | Ray::File[Ray::FileH]);

                return (file | rank) & ~AsBitBoard(sq);
            }

            std::unreachable();
        }

        struct MagicMask { BitBoard  Magic; BitBoard Mask; };
        struct MagicPair { MagicMask MMask; u32      Base; };

        constexpr Array<MagicPair, 2, 64> Magic = [] constexpr -> Array<MagicPair, 2, 64>
        {
            Array<MagicPair, 2, 64> result {};

            for (Square sq = A1; sq < InvalidSquare; ++sq) {
                const auto& [magicBishop, baseBishop] = Bishop[sq];
                const auto& [magicRook  , baseRook  ] = Rook  [sq];

                result[0][sq] = MagicPair {
                    .MMask = MagicMask {
                        .Magic = magicBishop,
                        .Mask  = ~OccupationMask<PieceType::Bishop>(sq)
                    },
                    .Base = baseBishop
                };

                result[1][sq] = MagicPair {
                    .MMask = MagicMask {
                        .Magic = magicRook,
                        .Mask  = ~OccupationMask<PieceType::Rook  >(sq)
                    },
                    .Base = baseRook
                };
            }

            return result;
        }();

    } // StockDory::BlackMagic

    namespace Attack
    {

        constexpr Array<BitBoard, 2, 64> Pawn {
            0x0000000000000200, 0x0000000000000500, 0x0000000000000a00, 0x0000000000001400,
            0x0000000000002800, 0x0000000000005000, 0x000000000000a000, 0x0000000000004000,
            0x0000000000020000, 0x0000000000050000, 0x00000000000a0000, 0x0000000000140000,
            0x0000000000280000, 0x0000000000500000, 0x0000000000a00000, 0x0000000000400000,
            0x0000000002000000, 0x0000000005000000, 0x000000000a000000, 0x0000000014000000,
            0x0000000028000000, 0x0000000050000000, 0x00000000a0000000, 0x0000000040000000,
            0x0000000200000000, 0x0000000500000000, 0x0000000a00000000, 0x0000001400000000,
            0x0000002800000000, 0x0000005000000000, 0x000000a000000000, 0x0000004000000000,
            0x0000020000000000, 0x0000050000000000, 0x00000a0000000000, 0x0000140000000000,
            0x0000280000000000, 0x0000500000000000, 0x0000a00000000000, 0x0000400000000000,
            0x0002000000000000, 0x0005000000000000, 0x000a000000000000, 0x0014000000000000,
            0x0028000000000000, 0x0050000000000000, 0x00a0000000000000, 0x0040000000000000,
            0x0200000000000000, 0x0500000000000000, 0x0a00000000000000, 0x1400000000000000,
            0x2800000000000000, 0x5000000000000000, 0xa000000000000000, 0x4000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,

            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
            0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
            0x0000000000000002, 0x0000000000000005, 0x000000000000000a, 0x0000000000000014,
            0x0000000000000028, 0x0000000000000050, 0x00000000000000a0, 0x0000000000000040,
            0x0000000000000200, 0x0000000000000500, 0x0000000000000a00, 0x0000000000001400,
            0x0000000000002800, 0x0000000000005000, 0x000000000000a000, 0x0000000000004000,
            0x0000000000020000, 0x0000000000050000, 0x00000000000a0000, 0x0000000000140000,
            0x0000000000280000, 0x0000000000500000, 0x0000000000a00000, 0x0000000000400000,
            0x0000000002000000, 0x0000000005000000, 0x000000000a000000, 0x0000000014000000,
            0x0000000028000000, 0x0000000050000000, 0x00000000a0000000, 0x0000000040000000,
            0x0000000200000000, 0x0000000500000000, 0x0000000a00000000, 0x0000001400000000,
            0x0000002800000000, 0x0000005000000000, 0x000000a000000000, 0x0000004000000000,
            0x0000020000000000, 0x0000050000000000, 0x00000a0000000000, 0x0000140000000000,
            0x0000280000000000, 0x0000500000000000, 0x0000a00000000000, 0x0000400000000000,
            0x0002000000000000, 0x0005000000000000, 0x000a000000000000, 0x0014000000000000,
            0x0028000000000000, 0x0050000000000000, 0x00a0000000000000, 0x0040000000000000
        };

        constexpr Array<BitBoard, 64> Knight {
            0x0000000000020400, 0x0000000000050800, 0x00000000000A1100, 0x0000000000142200,
            0x0000000000284400, 0x0000000000508800, 0x0000000000A01000, 0x0000000000402000,
            0x0000000002040004, 0x0000000005080008, 0x000000000A110011, 0x0000000014220022,
            0x0000000028440044, 0x0000000050880088, 0x00000000A0100010, 0x0000000040200020,
            0x0000000204000402, 0x0000000508000805, 0x0000000A1100110A, 0x0000001422002214,
            0x0000002844004428, 0x0000005088008850, 0x000000A0100010A0, 0x0000004020002040,
            0x0000020400040200, 0x0000050800080500, 0x00000A1100110A00, 0x0000142200221400,
            0x0000284400442800, 0x0000508800885000, 0x0000A0100010A000, 0x0000402000204000,
            0x0002040004020000, 0x0005080008050000, 0x000A1100110A0000, 0x0014220022140000,
            0x0028440044280000, 0x0050880088500000, 0x00A0100010A00000, 0x0040200020400000,
            0x0204000402000000, 0x0508000805000000, 0x0A1100110A000000, 0x1422002214000000,
            0x2844004428000000, 0x5088008850000000, 0xA0100010A0000000, 0x4020002040000000,
            0x0400040200000000, 0x0800080500000000, 0x1100110A00000000, 0x2200221400000000,
            0x4400442800000000, 0x8800885000000000, 0x100010A000000000, 0x2000204000000000,
            0x0004020000000000, 0x0008050000000000, 0x00110A0000000000, 0x0022140000000000,
            0x0044280000000000, 0x0088500000000000, 0x0010A00000000000, 0x0020400000000000
        };

        constexpr Array<BitBoard, 64> King {
            0x0000000000000302, 0x0000000000000705, 0x0000000000000E0A, 0x0000000000001C14,
            0x0000000000003828, 0x0000000000007050, 0x000000000000E0A0, 0x000000000000C040,
            0x0000000000030203, 0x0000000000070507, 0x00000000000E0A0E, 0x00000000001C141C,
            0x0000000000382838, 0x0000000000705070, 0x0000000000E0A0E0, 0x0000000000C040C0,
            0x0000000003020300, 0x0000000007050700, 0x000000000E0A0E00, 0x000000001C141C00,
            0x0000000038283800, 0x0000000070507000, 0x00000000E0A0E000, 0x00000000C040C000,
            0x0000000302030000, 0x0000000705070000, 0x0000000E0A0E0000, 0x0000001C141C0000,
            0x0000003828380000, 0x0000007050700000, 0x000000E0A0E00000, 0x000000C040C00000,
            0x0000030203000000, 0x0000070507000000, 0x00000E0A0E000000, 0x00001C141C000000,
            0x0000382838000000, 0x0000705070000000, 0x0000E0A0E0000000, 0x0000C040C0000000,
            0x0003020300000000, 0x0007050700000000, 0x000E0A0E00000000, 0x001C141C00000000,
            0x0038283800000000, 0x0070507000000000, 0x00E0A0E000000000, 0x00C040C000000000,
            0x0302030000000000, 0x0705070000000000, 0x0E0A0E0000000000, 0x1C141C0000000000,
            0x3828380000000000, 0x7050700000000000, 0xE0A0E00000000000, 0xC040C00000000000,
            0x0203000000000000, 0x0507000000000000, 0x0A0E000000000000, 0x141C000000000000,
            0x2838000000000000, 0x5070000000000000, 0xA0E0000000000000, 0x40C0000000000000
        };

        u32 SlidingIndex(const PieceType type, const Square sq, const BitBoard occ)
        {
            const auto& [mMask, base] = BlackMagic::Magic[type - Bishop][sq];
            const auto& [magic, mask] = mMask;

            return base + static_cast<i32>(
                (
                    (occ | mask) * magic
                )
                >> (64 - BlackMagic::PieceK[type - Bishop])
            );
        }

        Array<BitBoard, 87988> SlidingInternal = [] -> Array<BitBoard, 87988>
        {
            Array<BitBoard, 87988> result {};

            struct Direction { i08 dF, dR; };

            for (const PieceType type : { Bishop, Rook }) {
                constexpr Array<Direction, 2, 4> Deltas {{
                    {{ {1, 1}, {1, -1}, {-1, -1}, {-1, 1} }},
                    {{ {1, 0}, {0, -1}, {-1,  0}, { 0, 1} }}
                }};

                const auto& magic = BlackMagic::Magic[type - Bishop];
                const auto& delta = Deltas           [type - Bishop];

                for (u08 file = 0; file < 8; file++)
                for (u08 rank = 0; rank < 8; rank++) {
                    const auto sq = static_cast<Square>(rank * 8 + file);

                    const BitBoard mask = ~(magic[sq].MMask.Mask);

                    BitBoard occ {};

                    while (true) {
                        BitBoard moves {};

                        for (auto [dF, dR] : delta) {
                            i08 tFile = file;
                            i08 tRank = rank;

                            auto tSq = static_cast<Square>(tRank * 8 + tFile);

                            while (!Get(occ, tSq)) {
                                const i08 tDF = tFile + dF;
                                const i08 tDR = tRank + dR;

                                if (tDF > 7 || tDF < 0) break;
                                if (tDR > 7 || tDR < 0) break;

                                tFile = tDF;
                                tRank = tDR;

                                tSq = static_cast<Square>(tRank * 8 + tFile);

                                moves |= AsBitBoard(tSq);
                            }
                        }

                        result[SlidingIndex(type, sq, occ)] = moves;

                        occ = (occ - mask) & mask;

                        if (Count(occ) == 0) break;
                    }
                }
            }

            return result;
        }();

        template<PieceType type>
        BitBoard Sliding(const Square sq, const BitBoard occ) { return SlidingInternal[SlidingIndex(type, sq, occ)]; }

    } // StockDory::Attack

    namespace Ray
    {

        Array<BitBoard, 64, 64> Between = [] -> Array<BitBoard, 64, 64>
        {
            Array<BitBoard, 64, 64> result {};

            for (Square origin = A1; origin < InvalidSquare; ++origin) {
                const u08 oFile = origin % 8;
                const u08 oRank = origin / 8;

                for (Square target = A1; target < InvalidSquare; ++target) {
                    if (origin == target) continue;

                    const u08 tFile = target % 8;
                    const u08 tRank = target / 8;

                    const BitBoard occ = AsBitBoard(origin) | AsBitBoard(target);

                    if (oFile == tFile || oRank == tRank) {
                        result[origin][target] = Attack::Sliding<Rook>(origin, occ) |
                                                 Attack::Sliding<Rook>(target, occ) ;

                        continue;
                    }

                    const i08 fileDistance = abs(static_cast<i08>(oFile) - static_cast<i08>(tFile));
                    const i08 rankDistance = abs(static_cast<i08>(oRank) - static_cast<i08>(tRank));

                    if (fileDistance != rankDistance) continue;

                    result[origin][target] = Attack::Sliding<Bishop>(origin, occ) |
                                             Attack::Sliding<Bishop>(target, occ) ;
                }
            }

            return result;
        }();

    } // StockDory::Ray

} // StockDory

#endif //STOCKDORY_BITBOARD_H
