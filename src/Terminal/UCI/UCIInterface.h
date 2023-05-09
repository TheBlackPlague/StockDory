//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCIINTERFACE_H
#define STOCKDORY_UCIINTERFACE_H

#include <vector>
#include <thread>
#include <iostream>
#include <unordered_map>
#include <functional>

#include "../../External/strutil.h"

#include "../../Backend/Board.h"
#include "../../Backend/Information.h"
#include "../../Backend/Util.h"

#include "../../Engine/Evaluation.h"

namespace StockDory
{

    class UCIInterface
    {

        using Arguments     = std::vector<std::string>;
        using Handler       = std::function<void(const Arguments&)>;
        using HandlerSwitch = std::unordered_map<std::string, Handler>;

        private:
            static bool       Running;
            static bool SearchRunning;

            static bool UciPrompted;

            static HandlerSwitch BasicCommandHandler;

            static Board MainBoard;

        public:
            static void Launch()
            {
                RegisterBasicCommands();

                std::string input;
                while (Running && std::getline(std::cin, input)) HandleInput(input);
            }

        private:
            static void RegisterBasicCommands()
            {
                BasicCommandHandler.emplace("uci"     , [](const Arguments&     ) { Uci           (    ); });
                BasicCommandHandler.emplace("quit"    , [](const Arguments&     ) { Quit          (    ); });
                BasicCommandHandler.emplace("isready" , [](const Arguments&     ) { IsReady       (    ); });
                BasicCommandHandler.emplace("info"    , [](const Arguments&     ) { Info          (    ); });
                BasicCommandHandler.emplace("position", [](const Arguments& args) { HandlePosition(args); });
            }

            static void HandleInput(const std::string& input)
            {
                Arguments tokens = strutil::split(input, ' ');
                std::string command = strutil::to_lower(tokens[0]);
                if (!BasicCommandHandler.contains(command)) return;

                Arguments args = {tokens.begin() + 1, tokens.end()};
                BasicCommandHandler[command](args);
            }

            static void Uci()
            {
                if (UciPrompted) return;

                std::cout << "id name " << Title << " " << Version << std::endl;
                std::cout << "id author " << Author << std::endl;
                std::cout << "id license " << License << std::endl;
                std::cout << "uciok" << std::endl;
                UciPrompted = true;
            }

            static void IsReady()
            {
                if (!UciPrompted) return;

                std::cout << "readyok" << std::endl;
            }

            static void Quit()
            {
                Running = false;
            }

            static void Info()
            {
                if (!UciPrompted || SearchRunning) return;

                MainBoard.LoadForEvaluation();
                int32_t evaluation = StockDory::Evaluation::Evaluate(MainBoard.ColorToMove());
                std::cout << "FEN: " << MainBoard.Fen() << std::endl;
                std::cout << "Hash: " << Util::ToHex(MainBoard.Zobrist()) << std::endl;
                std::cout << "Evaluation: " << evaluation << std::endl;
            }

            static void HandlePosition(const Arguments& args)
            {
                if (!UciPrompted) return;

                if (strutil::compare_ignore_case(args[0], "fen")) {
                    Arguments fenToken = {args.begin() + 1, args.begin() + 7};
                    std::string fen = strutil::join(fenToken, " ");
                    MainBoard = Board(fen);
                } else if (strutil::compare_ignore_case(args[0], "startpos")) {
                    MainBoard = Board();
                } else {
                    std::cout << "Error: Invalid position command." << std::endl;
                    return;
                }

                if (args.size() > 6) {
                    Arguments movesToken = {args.begin() + 7, args.end()};
                    // Handle moves.
                }
            }

    };

} // StockDory

bool StockDory::UCIInterface::      Running = true ;
bool StockDory::UCIInterface::SearchRunning = false;

bool StockDory::UCIInterface::UciPrompted = false;

StockDory::UCIInterface::HandlerSwitch StockDory::UCIInterface::BasicCommandHandler =
        StockDory::UCIInterface::HandlerSwitch();
StockDory::Board StockDory::UCIInterface::MainBoard = StockDory::Board();

#endif //STOCKDORY_UCIINTERFACE_H
