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

#include "UCISearch.h"
#include "UCIOption.h"
//#include "../Perft/PerftRunner.h"

namespace StockDory
{

    class UCIInterface
    {

        using Arguments      = std::vector<std::string>;
        using CommandHandler = std::function<void(const Arguments&)>;
        using CommandSwitch  = std::unordered_map<std::string, CommandHandler>;
        using OptionSwitch   = std::unordered_map<std::string, std::shared_ptr<UCIOptionBase>>;

        private:
            static bool Running;

            static bool UciPrompted;

            static CommandSwitch UCICommandSwitch;
            static OptionSwitch  UCIOptionSwitch;

            static Board             MainBoard  ;
            static RepetitionHistory MainHistory;

            static UCISearch Search;

        public:
            static void Launch()
            {
                RegisterOptions();
                RegisterCommands();

                std::string input;
                while (Running && std::getline(std::cin, input)) HandleInput(input);
            }

        private:
            static void RegisterCommands()
            {
                UCICommandSwitch.emplace("uci"       , [](const Arguments&     ) { Uci           (    ); });
                UCICommandSwitch.emplace("setoption" , [](const Arguments& args) { SetOption     (args); });
                UCICommandSwitch.emplace("quit"      , [](const Arguments&     ) { Quit          (    ); });
                UCICommandSwitch.emplace("ucinewgame", [](const Arguments&     ) { UciNewGame    (    ); });
                UCICommandSwitch.emplace("isready"   , [](const Arguments&     ) { IsReady       (    ); });
                UCICommandSwitch.emplace("info"      , [](const Arguments& args) { Info          (args); });
                UCICommandSwitch.emplace("position"  , [](const Arguments& args) { HandlePosition(args); });
                UCICommandSwitch.emplace("go"        , [](const Arguments& args) { HandleGo      (args); });
                UCICommandSwitch.emplace("stop"      , [](const Arguments&     ) { HandleStop    (    ); });
            }

            static void RegisterOptions()
            {
                std::shared_ptr <UCIOption<uint64_t>> hash =
                std::make_shared<UCIOption<uint64_t>>
                ("Hash", 16, 1, 16384, [](const uint64_t& value) {
                    if        (value < 1    ) {
                        std::cerr << "ERROR: Hash must be at least 1 MB" << std::endl;
                        return;
                    } else if (value > 16384) {
                        std::cerr << "ERROR: Hash must be at most 16 GB" << std::endl;
                        return;
                    }

                    TTable.Resize(value * MB);
                });

                std::shared_ptr <UCIOption<uint8_t >> threads =
                std::make_shared<UCIOption<uint8_t >>
                ("Threads", 1, 1, 128, [](const  uint8_t& value) {
                    if (value != 1) {
                        std::cerr << "ERROR: Multithreading is not supported yet" << std::endl;
                        return;
                    }
                });

                UCIOptionSwitch.emplace(hash   ->GetName(), hash   );
                UCIOptionSwitch.emplace(threads->GetName(), threads);
            }

            static void HandleInput(const std::string& input)
            {
                const Arguments tokens = strutil::split(input, ' ');
                const std::string command = strutil::to_lower(tokens[0]);
                if (!UCICommandSwitch.contains(command)) return;

                const Arguments args = {tokens.begin() + 1, tokens.end()};
                UCICommandSwitch[command](args);
            }

            static void Uci()
            {
                if (UciPrompted) return;

                std::stringstream ss;
                ss << "id name " << Title << " " << Version << "\n";
                ss << "id author " << Author << "\n";
                ss << "id license " << License << "\n";

                for (const auto& [_, option] : UCIOptionSwitch)
                    ss << option->Log() << "\n";

                ss << "uciok";

                std::cout << ss.str() << std::endl;
                UciPrompted = true;
            }

            static void SetOption(const Arguments& args)
            {
                if (!UciPrompted || args.size() < 4) return;

                if (!UCIOptionSwitch.contains(args[1])) return;

                const std::vector<std::string> parameterLeading = {args.begin() + 3, args.end()};
                const std::string parameter = strutil::join(parameterLeading, "");

                UCIOptionSwitch[args[1]]->Set(parameter);
            }

            static void UciNewGame()
            {
                if (!UciPrompted) return;

                Search.Stop();

                MainBoard = Board();
                MainHistory = RepetitionHistory(MainBoard.Zobrist());
                TTable.Clear();
            }

            static void IsReady()
            {
                if (!UciPrompted) return;

                std::cout << "readyok" << std::endl;
            }

            static void Quit()
            {
                Search.Stop();
                Running = false;
            }

            static void Info(const Arguments& args)
            {
                if (!UciPrompted || Search.IsRunning()) return;

                MainBoard.LoadForEvaluation();
                const int32_t evaluation = StockDory::Evaluation::Evaluate(MainBoard.ColorToMove());

                std::stringstream ss;
                ss << "FEN: " << MainBoard.Fen() << "\n";
                ss << "Hash: " << Util::ToHex(MainBoard.Zobrist()) << "\n";
                ss << "Evaluation: " << evaluation;

                if (!args.empty() && strutil::compare_ignore_case(args[0], "moves")) {
                    ss << "\nMoves: ";
                    if (MainBoard.ColorToMove() == White) {
                        OrderedMoveList<White> moves(MainBoard, 0,
                                                     KillerTable(), HistoryTable(), Move());

                        for (uint8_t i = 0; i < moves.Count(); i++) ss << "\n" << moves[i].ToString();
                    } else {
                        OrderedMoveList<Black> moves(MainBoard, 0,
                                                     KillerTable(), HistoryTable(), Move());

                        for (uint8_t i = 0; i < moves.Count(); i++) ss << "\n" << moves[i].ToString();
                    }
                }

                std::cout << ss.str() << std::endl;
            }

            static void HandlePosition(const Arguments& args)
            {
                if (!UciPrompted) return;

                uint8_t moveStrIndex = 2;
                if (strutil::compare_ignore_case(args[0], "fen")) {
                    const Arguments   fenToken = {args.begin() + 1, args.begin() + 7};
                    const std::string fen      = strutil::join(fenToken, " ");
                    MainBoard   = Board(fen);
                    MainHistory = RepetitionHistory(MainBoard.Zobrist());
                    moveStrIndex = 8;
                } else if (strutil::compare_ignore_case(args[0], "startpos")) {
                    MainBoard   = Board();
                    MainHistory = RepetitionHistory(MainBoard.Zobrist());
                } else return;

                if (args.size() >= moveStrIndex &&
                    strutil::compare_ignore_case(args[moveStrIndex - 1], "moves")) {
                    const Arguments movesToken = {args.begin() + moveStrIndex, args.end()};

                    for (const std::string& moveStr : movesToken) {
                        const Move move = Move::FromString(moveStr);
                        MainBoard.Move<ZOBRIST>(move.From(), move.To(), move.Promotion());
                        MainHistory.Push(MainBoard.Zobrist());
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
                if (!UciPrompted || Search.IsRunning()) return;

//                if (args.size() > 1 && strutil::compare_ignore_case(args[0], "perft")) {
//                    const auto depth = static_cast<uint8_t>(std::stoull(args[1]));
//                    StockDory::PerftRunner::SetBoard(MainBoard);
//                    StockDory::PerftRunner::Perft<true>(depth);
//                    return;
//                }

                using MS = StockDory::TimeControl::Milliseconds;

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

                        .MovesToGo = TokenToValue<uint16_t>(args, "movestogo", 0)
                    };

                    timeControl = TimeControl(MainBoard, timeData);
                }

                Search.Stop();
                Search = UCISearch(MainBoard, timeControl, MainHistory);
                Search.Start(depth);
            }

            static void HandleStop()
            {
                if (!UciPrompted || !Search.IsRunning()) return;

                Search.Stop();
            }

    };

} // StockDory

bool StockDory::UCIInterface::Running = true;

bool StockDory::UCIInterface::UciPrompted = false;

StockDory::UCIInterface::CommandSwitch StockDory::UCIInterface::UCICommandSwitch =
        StockDory::UCIInterface::CommandSwitch();

StockDory::UCIInterface::OptionSwitch StockDory::UCIInterface::UCIOptionSwitch =
        StockDory::UCIInterface::OptionSwitch();

StockDory::Board             StockDory::UCIInterface::MainBoard   = StockDory::Board();
StockDory::RepetitionHistory StockDory::UCIInterface::MainHistory =
        StockDory::RepetitionHistory(MainBoard.Zobrist());

StockDory::UCISearch StockDory::UCIInterface::Search = StockDory::UCISearch();

#endif //STOCKDORY_UCIINTERFACE_H
