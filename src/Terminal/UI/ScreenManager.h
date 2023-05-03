//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_SCREENMANAGER_H
#define STOCKDORY_SCREENMANAGER_H

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

#include "../../External/strutil.h"

#include "../../Backend/Board.h"

#include "../../Engine/Evaluation.h"

using namespace ftxui;
using UIColor = ftxui::Color;

namespace StockDory
{

    class ScreenManager
    {

        private:
            static Element     BoardUI;

            static std::string Title  ;

            constexpr static std::array<char16_t, 7> PieceSymbol {
                u'♙', u'♞', u'♝', u'♜', u'♛', u'♚', u' '
            };

            static Element DrawPiece(const Piece piece)
            {
                std::wstring drawing;
                drawing += FirstLetter(piece);
                drawing += PieceSymbol[piece];
                return text(drawing);
            }

        public:
            static void DrawBoard(const Board& board)
            {
                const uint8_t BoxSize = 3          ;
                const uint8_t Width   = 8 * BoxSize;
                const uint8_t Height  = 8 * BoxSize;

                const UIColor LightSq = UIColor(251, 173, 108);
                const UIColor  DarkSq = UIColor(211, 133, 046);

                const std::string Empty   = "  ";
                const std::string Padding = "       ";

                std::vector<Element> vBoxContainer;

                bool verticallyPadded = false;
                bool light            = true ;
                for (uint8_t v = 0; v <= Height; v += BoxSize) {
                    // Initial vertical padding.
                    if (v == 0 && !verticallyPadded) {
                        for (uint8_t i = 0; i < BoxSize - 2; i++) {
                            std::vector<Element> hBoxContainer;

                            // Horizontal padding:
                            hBoxContainer.push_back(text(Padding + Empty + " "));

                            for (uint8_t h = 0; h <= Width; h += BoxSize) {
                                // Horizontal padding:
                                if (h == Width) {
                                    for (uint8_t j = 0; j < BoxSize - 1; j++) {
                                        Element emptyPadding = text(Padding);
                                        if (j == BoxSize - 2) emptyPadding = text(" ");
                                        hBoxContainer.push_back(emptyPadding);
                                    }

                                    continue;
                                }

                                for (uint8_t j = 0; j < BoxSize; j++) {
                                    Element emptyPadding = text(Empty);
                                    hBoxContainer.push_back(emptyPadding);
                                }
                            }

                            vBoxContainer.push_back(hbox(hBoxContainer));
                        }

                        v -= BoxSize;
                        verticallyPadded = true;
                        continue;
                    }

                    const uint8_t rank = 7 - (v / BoxSize);

                    // Repeat Box Size times to create a square (rather than horizontal rectangle):
                    for (uint8_t i = 0; i < BoxSize; i++) {
                        std::vector<Element> hBoxContainer;

                        // Draw horizontal labels at the end.
                        if (v == Height) {
                            // No need to pad vertically more than BoxSize - 1.
                            if (i == BoxSize - 1) continue;

                            // Horizontal padding:
                            hBoxContainer.push_back(text(Padding + Empty));

                            for (uint8_t h = 0; h < Width; h += BoxSize) {
                                const uint8_t file = h / BoxSize;

                                for (int j = 0; j < BoxSize; j++) {
                                    Element fileLabel = text(Empty);

                                    if (i == BoxSize / 2 && j == BoxSize / 2) {
                                        std::string label;
                                        label += static_cast<char>(tolower(FILE_CHAR[file]));
                                        label += ' ';
                                        fileLabel = text(label);
                                    }

                                    hBoxContainer.push_back(fileLabel);
                                }
                            }

                            vBoxContainer.push_back(hbox(hBoxContainer));
                            continue;
                        }

                        // Draw vertical labels.
                        Element rankLabel = text(Padding + Empty);
                        if (i == BoxSize / 2) rankLabel = text(Padding + std::to_string(rank + 1) + " ");
                        hBoxContainer.push_back(rankLabel);

                        for (uint8_t h = 0; h <= Width; h += BoxSize) {
                            // Horizontal padding:
                            if (h == Width) {
                                for (uint8_t j = 0; j < BoxSize - 1; j++) {
                                    Element emptyPadding = text(Empty);
                                    if (j == BoxSize - 2) emptyPadding = text(" ");

                                    hBoxContainer.push_back(emptyPadding);
                                }

                                continue;
                            }

                            const uint8_t  file = h / BoxSize;
                            const auto       sq = static_cast<Square>(rank * 8 + file);
                            const PieceColor pc = board[sq];

                            const UIColor colorSq = light ? LightSq : DarkSq;
                            const auto    bgColor = bgcolor(colorSq);
                            const auto    pcColor = pc.Color() == White ?
                                    color(UIColor::White) :
                                    color(UIColor::Black) ;

                            // Repeat Box Size times to create a square (rather than vertical rectangle):
                            for (uint8_t j = 0; j < BoxSize; j++) {
                                Element pixelInformation = text(Empty);

                                // Draw the piece in the center of the square:
                                if (i == BoxSize / 2 && j == BoxSize / 2)
                                    pixelInformation = DrawPiece(pc.Piece()) | pcColor;

                                hBoxContainer.push_back(pixelInformation | bgColor);
                            }

                            light = !light;
                        }

                        vBoxContainer.push_back(hbox(hBoxContainer));
                    }

                    // To ensure the next rank starts with a different order.
                    light = !light;
                }

                vBoxContainer.push_back(separator());

                int32_t evaluation = StockDory::Evaluation::Evaluate(board);

                Element fen         = text("FEN: "  + board.Fen()) | center;
                Element hashHex     = text("Hash: " + StockDory::Util::ToHex(board.Zobrist())) | center;
                Element evalDisplay = text("Evaluation: " + strutil::to_string(evaluation)) | center;
                vBoxContainer.push_back(fen);
                vBoxContainer.push_back(hashHex);
                vBoxContainer.push_back(evalDisplay);

                BoardUI = vbox(vBoxContainer);
            }

            static void Refresh()
            {
                auto ui = hbox({
                    BoardUI | flex,
//                    separator()
                });

                auto windowOnScreen = window(text(Title), ui);

                Screen screen = Screen::Create(Dimension::Fit(windowOnScreen));
                Render(screen, windowOnScreen);
                screen.Print();
                std::cout << std::endl;
            }

    };

} // StockDory

std::string    StockDory::ScreenManager::Title   = "StockDory";
ftxui::Element StockDory::ScreenManager::BoardUI = vbox();

#endif //STOCKDORY_SCREENMANAGER_H
