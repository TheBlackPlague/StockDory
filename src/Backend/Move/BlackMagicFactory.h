//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_BLACKMAGICFACTORY_H
#define STOCKDORY_BLACKMAGICFACTORY_H

#include <array>
#include <cstdint>

#include "../Type/BitBoard.h"
#include "../Type/Piece.h"

namespace StockDory
{

    class BlackMagicFactory
    {

        public:
            constexpr static std::array<BitBoard, 8> Horizontal {
                0x0101010101010101, 0x0202020202020202, 0x0404040404040404, 0x0808080808080808,
                0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080
            };

            constexpr static std::array<BitBoard, 8> Vertical {
                0x00000000000000FF, 0x000000000000FF00, 0x0000000000FF0000, 0x00000000FF000000,
                0x000000FF00000000, 0x0000FF0000000000, 0x00FF000000000000, 0xFF00000000000000
            };

            constexpr static BitBoard Edged = Horizontal[0] | Horizontal[7] | Vertical[0] | Vertical[7];

        private:
            constexpr static std::array<uint8_t, 2> PieceValue { 9, 12 };

            constexpr static std::array<std::pair<BitBoard, int32_t>, 64> RookMagicConst {
                std::pair(0x80280013FF84FFFF, 10890), std::pair(0x5FFBFEFDFEF67FFF, 50579),
                std::pair(0xFFEFFAFFEFFDFFFF, 62020), std::pair(0x003000900300008A, 67322),
                std::pair(0x0050028010500023, 80251), std::pair(0x0020012120A00020, 58503),
                std::pair(0x0030006000C00030, 51175), std::pair(0x0058005806B00002, 83130),
                std::pair(0x7FBFF7FBFBEAFFFC, 50430), std::pair(0x0000140081050002, 21613),
                std::pair(0x0000180043800048, 72625), std::pair(0x7FFFE800021FFFB8, 80755),
                std::pair(0xFFFFCFFE7FCFFFAF, 69753), std::pair(0x00001800C0180060, 26973),
                std::pair(0x4F8018005FD00018, 84972), std::pair(0x0000180030620018, 31958),
                std::pair(0x00300018010C0003, 69272), std::pair(0x0003000C0085FFFF, 48372),
                std::pair(0xFFFDFFF7FBFEFFF7, 65477), std::pair(0x7FC1FFDFFC001FFF, 43972),
                std::pair(0xFFFEFFDFFDFFDFFF, 57154), std::pair(0x7C108007BEFFF81F, 53521),
                std::pair(0x20408007BFE00810, 30534), std::pair(0x0400800558604100, 16548),
                std::pair(0x0040200010080008, 46407), std::pair(0x0010020008040004, 11841),
                std::pair(0xFFFDFEFFF7FBFFF7, 21112), std::pair(0xFEBF7DFFF8FEFFF9, 44214),
                std::pair(0xC00000FFE001FFE0, 57925), std::pair(0x4AF01F00078007C3, 29574),
                std::pair(0xBFFBFAFFFB683F7F, 17309), std::pair(0x0807F67FFA102040, 40143),
                std::pair(0x200008E800300030, 64659), std::pair(0x0000008780180018, 70469),
                std::pair(0x0000010300180018, 62917), std::pair(0x4000008180180018, 60997),
                std::pair(0x008080310005FFFA, 18554), std::pair(0x4000188100060006, 14385),
                std::pair(0xFFFFFF7FFFBFBFFF,     0), std::pair(0x0000802000200040, 38091),
                std::pair(0x20000202EC002800, 25122), std::pair(0xFFFFF9FF7CFFF3FF, 60083),
                std::pair(0x000000404B801800, 72209), std::pair(0x2000002FE03FD000, 67875),
                std::pair(0xFFFFFF6FFE7FCFFD, 56290), std::pair(0xBFF7EFFFBFC00FFF, 43807),
                std::pair(0x000000100800A804, 73365), std::pair(0x6054000A58005805, 76398),
                std::pair(0x0829000101150028, 20024), std::pair(0x00000085008A0014,  9513),
                std::pair(0x8000002B00408028, 24324), std::pair(0x4000002040790028, 22996),
                std::pair(0x7800002010288028, 23213), std::pair(0x0000001800E08018, 56002),
                std::pair(0xA3A80003F3A40048, 22809), std::pair(0x2003D80000500028, 44545),
                std::pair(0xFFFFF37EEFEFDFBE, 36072), std::pair(0x40000280090013C1,  4750),
                std::pair(0xBF7FFEFFBFFAF71F,  6014), std::pair(0xFFFDFFFF777B7D6E, 36054),
                std::pair(0x48300007E8080C02, 78538), std::pair(0xAFE0000FFF780402, 28745),
                std::pair(0xEE73FFFBFFBB77FE,  8555), std::pair(0x0002000308482882,  1009)
            };

            constexpr static std::array<std::pair<BitBoard, int32_t>, 64> BishopMagicConst {
                std::pair(0xA7020080601803D8, 60984), std::pair(0x13802040400801F1, 66046),
                std::pair(0x0A0080181001F60C, 32910), std::pair(0x1840802004238008, 16369),
                std::pair(0xC03FE00100000000, 42115), std::pair(0x24C00BFFFF400000,   835),
                std::pair(0x0808101F40007F04, 18910), std::pair(0x100808201EC00080, 25911),
                std::pair(0xFFA2FEFFBFEFB7FF, 63301), std::pair(0x083E3EE040080801, 16063),
                std::pair(0xC0800080181001F8, 17481), std::pair(0x0440007FE0031000, 59361),
                std::pair(0x2010007FFC000000, 18735), std::pair(0x1079FFE000FF8000, 61249),
                std::pair(0x3C0708101F400080, 68938), std::pair(0x080614080FA00040, 61791),
                std::pair(0x7FFE7FFF817FCFF9, 21893), std::pair(0x7FFEBFFFA01027FD, 62068),
                std::pair(0x53018080C00F4001, 19829), std::pair(0x407E0001000FFB8A, 26091),
                std::pair(0x201FE000FFF80010, 15815), std::pair(0xFFDFEFFFDE39FFEF, 16419),
                std::pair(0xCC8808000FBF8002, 59777), std::pair(0x7FF7FBFFF8203FFF, 16288),
                std::pair(0x8800013E8300C030, 33235), std::pair(0x0420009701806018, 15459),
                std::pair(0x7FFEFF7F7F01F7FD, 15863), std::pair(0x8700303010C0C006, 75555),
                std::pair(0xC800181810606000, 79445), std::pair(0x20002038001C8010, 15917),
                std::pair(0x087FF038000FC001,  8512), std::pair(0x00080C0C00083007, 73069),
                std::pair(0x00000080FC82C040, 16078), std::pair(0x000000407E416020, 19168),
                std::pair(0x00600203F8008020, 11056), std::pair(0xD003FEFE04404080, 62544),
                std::pair(0xA00020C018003088, 80477), std::pair(0x7FBFFE700BFFE800, 75049),
                std::pair(0x107FF00FE4000F90, 32947), std::pair(0x7F8FFFCFF1D007F8, 59172),
                std::pair(0x0000004100F88080, 55845), std::pair(0x00000020807C4040, 61806),
                std::pair(0x00000041018700C0, 73601), std::pair(0x0010000080FC4080, 15546),
                std::pair(0x1000003C80180030, 45243), std::pair(0xC10000DF80280050, 20333),
                std::pair(0xFFFFFFBFEFF80FDC, 33402), std::pair(0x000000101003F812, 25917),
                std::pair(0x0800001F40808200, 32875), std::pair(0x084000101F3FD208,  4639),
                std::pair(0x080000000F808081, 17077), std::pair(0x0004000008003F80, 62324),
                std::pair(0x08000001001FE040, 18159), std::pair(0x72DD000040900A00, 61436),
                std::pair(0xFFFFFEFFBFEFF81D, 57073), std::pair(0xCD8000200FEBF209, 61025),
                std::pair(0x100000101EC10082, 81259), std::pair(0x7FBAFFFFEFE0C02F, 64083),
                std::pair(0x7F83FFFFFFF07F7F, 56114), std::pair(0xFFF1FFFFFFF7FFC1, 57058),
                std::pair(0x0878040000FFE01F, 58912), std::pair(0x945E388000801012, 22194),
                std::pair(0x0840800080200FDA, 70880), std::pair(0x100000C05F582008, 11140)
            };

            using MagicPair = std::pair<BitBoard, BitBoard>;

            constexpr static auto RookOccupiedMask =
            [](const Square sq) constexpr {
                // Horizontal files & vertical ranks.
                const BitBoard h = Horizontal[sq % 8] & ~(Vertical[0]   |   Vertical[7]);
                const BitBoard v = Vertical  [sq / 8] & ~(Horizontal[0] | Horizontal[7]);

                // Occupied inside but not the square itself.
                return (h | v) & ~FromSquare(sq);
            };

            constexpr static auto BishopOccupiedMask =
            [](const Square sq) constexpr {
                const int h = sq % 8;
                const int v = sq / 8;

                // Simple ray cast.
                BitBoard ray = BBDefault;
                for (int i = 0; i < 8; i++) for (int j = 0; j < 8; j++) {
                    int dH = i - h;
                    int dV = j - v;

                    if (dH < 0) dH = -dH;
                    if (dV < 0) dV = -dV;

                    if (dH == dV && dV != 0) ray |= 1ULL << (j * 8 + i);
                }

                return ray & ~Edged;
            };

        public:
            constexpr static std::array<std::array<std::pair<MagicPair, int32_t>, 64>, 2> Magic = []() constexpr {
                std::array<std::array<std::pair<MagicPair, int32_t>, 64>, 2> temp = {};

                for (int h = 0; h < 8; h++) for (int v = 0; v < 8; v++) {
                    const auto sq = static_cast<Square>(v * 8 + h);
                    const std::pair<BitBoard, int32_t>& bishopConst = BishopMagicConst[sq];

                    // Black magic:
                    temp[0][sq] =
                            {{bishopConst.first, ~BishopOccupiedMask(sq)}, bishopConst.second};
                }

                for (int h = 0; h < 8; h++) for (int v = 0; v < 8; v++) {
                    const auto sq = static_cast<Square>(v * 8 + h);
                    const std::pair<BitBoard, int32_t>& rookConst = RookMagicConst[sq];

                    // Black magic:
                    temp[1][sq] =
                            {{rookConst.first, ~RookOccupiedMask(sq)}, rookConst.second};
                }

                return temp;
            }();

            constexpr static inline uint32_t MagicIndex(Piece p, Square sq, BitBoard occupied)
            {
                const std::array<std::pair<MagicPair, int32_t>, 64>& pieceMagic = Magic[p - 2];

                // Get magic:
                const std::pair<MagicPair, uint32_t>& pieceMagicAtSq = pieceMagic[sq];

                // Relevant occupied squares:
                const BitBoard relevantOccupied = occupied | pieceMagicAtSq.first.second;

                // Hash:
                const BitBoard hash = relevantOccupied * pieceMagicAtSq.first.first;

                // Offset applied:
                return pieceMagicAtSq.second + static_cast<int32_t>((hash >> (64 - PieceValue[p - 2])));
            }

    };

} // StockDory

#endif //STOCKDORY_BLACKMAGICFACTORY_H
