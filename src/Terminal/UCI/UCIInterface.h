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

#include "../../Engine/Search.h"

namespace StockDory
{

    class UCISearchLogger
    {

        public:
            static void LogDepthIteration(const uint8_t depth, const int32_t evaluation, const uint64_t nodes,
                                          const StockDory::TimeControl::Milliseconds time, const std::string& pv)
            {
                std::stringstream output;

                int64_t displayedTime = time.count();
                displayedTime = std::max(displayedTime, static_cast<int64_t>(1));

                const auto nps = static_cast<uint64_t>(static_cast<double>(nodes) /
                        (static_cast<double>(displayedTime) / static_cast<double>(1000)));

                output << "info ";
                output << "depth " << static_cast<uint16_t>(depth) << " ";
                output << "score cp " << evaluation << " ";
                output << "nodes " << nodes << " ";
                output << "time " << displayedTime << " ";
                output << "nps " << nps << " ";
                output << "pv " << pv;

                std::cout << output.str() << std::endl;
            }

            static void LogBestMove(const Move& move)
            {
                std::stringstream output;
                output << "bestmove " << move.ToString();

                std::cout << output.str() << std::endl;
            }

    };

    class UCIInterface
    {

        using Arguments     = std::vector<std::string>;
        using Handler       = std::function<void(const Arguments&)>;
        using HandlerSwitch = std::unordered_map<std::string, Handler>;

        private:
            static bool Running;

            static std::atomic<bool> SearchRunning;

            static bool UciPrompted;

            static HandlerSwitch BasicCommandHandler;

            static Board MainBoard;

            static std::thread SearchThread;

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
                BasicCommandHandler.emplace("go"      , [](const Arguments& args) { HandleGo      (args); });
            }

            static void HandleInput(const std::string& input)
            {
                const Arguments tokens = strutil::split(input, ' ');
                const std::string command = strutil::to_lower(tokens[0]);
                if (!BasicCommandHandler.contains(command)) return;

                const Arguments args = {tokens.begin() + 1, tokens.end()};
                BasicCommandHandler[command](args);
            }

            static void Uci()
            {
                if (UciPrompted) return;

                std::cout << "id name "     << Title                ;
                std::cout << " "            << Version  << std::endl;
                std::cout << "id author "   << Author   << std::endl;
                std::cout << "id license "  << License  << std::endl;
                std::cout << "uciok"                    << std::endl;
                UciPrompted = true;
            }

            static void IsReady()
            {
                if (!UciPrompted) return;

                std::cout << "readyok" << std::endl;
            }

            static void Quit()
            {
                if (SearchThread.joinable()) SearchThread.join();

                Running = false;
            }

            static void Info()
            {
                if (!UciPrompted || SearchRunning) return;

                MainBoard.LoadForEvaluation();
                const int32_t evaluation = StockDory::Evaluation::Evaluate(MainBoard.ColorToMove());
                std::cout << "FEN: " << MainBoard.Fen() << std::endl;
                std::cout << "Hash: " << Util::ToHex(MainBoard.Zobrist()) << std::endl;
                std::cout << "Evaluation: " << evaluation << std::endl;
            }

            static void HandlePosition(const Arguments& args)
            {
                if (!UciPrompted) return;

                uint8_t moveStrIndex = 2;
                if (strutil::compare_ignore_case(args[0], "fen")) {
                    const Arguments   fenToken = {args.begin() + 1, args.begin() + 7};
                    const std::string fen      = strutil::join(fenToken, " ");
                    MainBoard = Board(fen);
                    moveStrIndex = 8;
                } else if (strutil::compare_ignore_case(args[0], "startpos")) {
                    MainBoard = Board();
                } else return;

                if (args.size() >= moveStrIndex &&
                    strutil::compare_ignore_case(args[moveStrIndex - 1], "moves")) {
                    const Arguments movesToken = {args.begin() + moveStrIndex, args.end()};

                    for (const std::string& moveStr : movesToken) {
                        const Move move = Move::FromString(moveStr);
                        MainBoard.Move<ZOBRIST | NNUE>(move.From(), move.To(), move.Promotion());
                    }
                }
            }

            template<typename T>
            [[nodiscard]]
            static T TokenToValue(const Arguments& args, const std::string& token, const T defaultValue)
            {
                for (size_t i = 0; i < args.size(); i++)
                    if (strutil::compare_ignore_case(args[i], token) && i + 1 < args.size())
                        return static_cast<T>(std::stoull(args[i + 1]));

                return defaultValue;
            }

            static void HandleGo(const Arguments& args)
            {
                if (!UciPrompted || SearchRunning) return;

                using MS  = StockDory::TimeControl::Milliseconds;

                uint8_t     depth       = MaxDepth / 2;
                TimeControl timeControl               ;

                if        (args.size() == 2) {
                    if      (strutil::compare_ignore_case(args[0], "movetime"))
                        timeControl = TimeControl(std::stoull(args[1]));
                    else if (strutil::compare_ignore_case(args[0], "depth"   ))
                        depth = static_cast<uint8_t>(std::stoull(args[1]));
                } else if (args.size() >  2) {
                    const StockDory::TimeControl::TimeData timeData {
                        .WhiteTime      = MS(TokenToValue<uint64_t>(args, "wtime", 0)),
                        .BlackTime      = MS(TokenToValue<uint64_t>(args, "btime", 0)),
                        .WhiteIncrement = MS(TokenToValue<uint64_t>(args, "winc" , 0)),
                        .BlackIncrement = MS(TokenToValue<uint64_t>(args, "binc" , 0)),
                    };

                    timeControl = TimeControl(MainBoard, timeData);
                }

                std::cout << "Parsed depth: " << static_cast<uint16_t>(depth) << std::endl;

                SearchRunning = true;
                SearchThread = std::thread(
                    [](const TimeControl timeControl, const uint8_t depth) {
                        Search<UCISearchLogger> search(MainBoard, timeControl);
                        search.IterativeDeepening(depth);
                        SearchRunning = false;
                    },
                    timeControl, depth
                );
            }

    };

} // StockDory

bool StockDory::UCIInterface::Running = true;

std::atomic<bool> StockDory::UCIInterface::SearchRunning = false;

bool StockDory::UCIInterface::UciPrompted = false;

StockDory::UCIInterface::HandlerSwitch StockDory::UCIInterface::BasicCommandHandler =
        StockDory::UCIInterface::HandlerSwitch();

StockDory::Board StockDory::UCIInterface::MainBoard = StockDory::Board();

std::thread StockDory::UCIInterface::SearchThread = std::thread();

#endif //STOCKDORY_UCIINTERFACE_H
