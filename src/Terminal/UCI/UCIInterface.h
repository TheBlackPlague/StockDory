//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCIINTERFACE_H
#define STOCKDORY_UCIINTERFACE_H

#include <functional>
#include <iostream>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "Information.h"

#include "../../External/strutil.h"

#include "../../Backend/Board.h"
#include "../../Backend/Misc.h"

#include "../../Engine/Search2.h"

#include "../Perft/PerftRunner.h"

#include "UCIOption.h"
#include "UCISearchEventHandler.h"
#include "UCITime.h"

namespace StockDory
{

    class UCIInterface
    {

        using UCISearch = ThreadedSearch<UCISearchEventHandler>;

        using Arguments      = std::vector<std::string>;
        using CommandHandler = std::function<void(const Arguments&)>;
        using CommandSwitch  = std::unordered_map<std::string, CommandHandler>;
        using OptionSwitch   = std::unordered_map<std::string, std::shared_ptr<UCIOptionBase>>;

        static inline bool     Running =  true;
        static inline bool UCIPrompted = false;

        static inline CommandSwitch UCICommandSwitch = {};
        static inline OptionSwitch  UCIOptionSwitch  = {};

        static inline Board                     Board {};
        static inline RepetitionStack      Repetition {};
        static inline uint8_t         HalfMoveCounter {};

        static inline UCISearchEventHandler SearchEventHandler;

        static UCISearch Search;

        public:
        static void Launch()
        {
            Repetition.Push(Board.Zobrist());
            HalfMoveCounter = 1;

            RegisterOptions();
            RegisterCommands();

            std::string input;
            while (Running && std::getline(std::cin, input)) HandleInput(input);
        }

        private:
        static void RegisterCommands()
        {
            UCICommandSwitch.emplace("uci",        [](const Arguments&     ) { Uci();                });
            UCICommandSwitch.emplace("setoption",  [](const Arguments& args) { SetOption(args);      });
            UCICommandSwitch.emplace("quit",       [](const Arguments&     ) { Quit();               });
            UCICommandSwitch.emplace("ucinewgame", [](const Arguments&     ) { UciNewGame();         });
            UCICommandSwitch.emplace("isready",    [](const Arguments&     ) { IsReady();            });
            UCICommandSwitch.emplace("info",       [](const Arguments& args) { Info(args);           });
            UCICommandSwitch.emplace("position",   [](const Arguments& args) { HandlePosition(args); });
            UCICommandSwitch.emplace("go",         [](const Arguments& args) { HandleGo(args);       });
            UCICommandSwitch.emplace("stop",       [](const Arguments&     ) { HandleStop();         });
        }

        static void RegisterOptions()
        {
            auto hash =
                std::make_shared<UCIOption<size_t>>
                ("Hash", 16, 1, 16384, [](const size_t& value) -> void
                    {
                        if (value < 1) {
                            std::cerr << "ERROR: Hash must be at least 1 MB" << std::endl;
                            return;
                        }
                        if (value > 16384) {
                            std::cerr << "ERROR: Hash must be at most 16 GB" << std::endl;
                            return;
                        }

                        TT.Resize(value * MB);
                    }
                );

            auto threads =
                std::make_shared<UCIOption<size_t>>
                ("Threads", 1, 1, ThreadPool::HardwareLimit(), [](const size_t& value) -> void
                    {
                        if (value < 1) {
                            std::cerr << "ERROR: Maximum thread count must be at least 1" << std::endl;
                            return;
                        }

                        if (value > ThreadPool::HardwareLimit()) {
                            std::cerr << "ERROR: Maximum thread count exceeds number of logical processors" << std::endl;
                            return;
                        }

                        ThreadPool.Resize(value);

                        Evaluation::Initialize();
                    }
                );

            UCIOptionSwitch.emplace(   hash->GetName(), hash   );
            UCIOptionSwitch.emplace(threads->GetName(), threads);
        }

        static void HandleInput(const std::string& input)
        {
            const Arguments   tokens  = strutil::split(input, ' ');
            const std::string command = strutil::to_lower(tokens[0]);
            if (!UCICommandSwitch.contains(command)) return;

            const Arguments args = {tokens.begin() + 1, tokens.end()};
            UCICommandSwitch[command](args);
        }

        static void Uci()
        {
            if (UCIPrompted) return;

            std::stringstream ss;
            ss << "id name " << NAME << " " << VERSION << "\n";
            ss << "id nnue " << Evaluation::Name() << "\n";
            ss << "id author " << AUTHOR << "\n";
            ss << "id license " << LICENSE << "\n";

            for (const auto& option: UCIOptionSwitch | std::views::values)
                ss << option->Log() << "\n";

            ss << "uciok";

            std::cout << ss.str() << std::endl;
            UCIPrompted = true;
        }

        static void SetOption(const Arguments& args)
        {
            if (!UCIPrompted || args.size() < 4) return;

            if (!UCIOptionSwitch.contains(args[1])) return;

            const std::vector<std::string> parameterLeading = {args.begin() + 3, args.end()};
            const std::string              parameter        = strutil::join(parameterLeading, "");

            UCIOptionSwitch[args[1]]->Set(parameter);
        }

        static void UciNewGame()
        {
            if (!UCIPrompted) return;

            if (Search.Running()) Search.Stop();

            Board           = {};
            Repetition      = {};
            HalfMoveCounter =  1;

            Repetition.Push(Board.Zobrist());

            TT.Clear();
        }

        static void IsReady()
        {
            if (!UCIPrompted) return;

            std::cout << "readyok" << std::endl;
        }

        static void Quit()
        {
            if (Search.Running()) Search.Stop();

            while (Search.Running()) Sleep(1);

            Running = false;
        }

        static void Info(const Arguments& args)
        {
            if (!UCIPrompted || Search.Running()) return;

            Board.LoadForEvaluation();

            const int32_t evaluation = Evaluation::Evaluate(Board.ColorToMove());

            std::stringstream ss;
            ss << "FEN: " << Board.Fen() << "\n";
            ss << "Hash: " << ToHex(Board.Zobrist()) << "\n";
            ss << "Evaluation: " << evaluation;

            if (!args.empty() && strutil::compare_ignore_case(args[0], "moves")) {
                ss << "\nMoves: ";
                if (Board.ColorToMove() == White) {
                    OrderedMoveList<White> moves (Board, 0, {}, {}, {});

                    for (uint8_t i = 0; i < moves.Count(); i++) ss << "\n" << moves[i].ToString();
                } else {
                    OrderedMoveList<Black> moves (Board, 0, {}, {}, {});

                    for (uint8_t i = 0; i < moves.Count(); i++) ss << "\n" << moves[i].ToString();
                }
            }

            std::cout << ss.str() << std::endl;
        }

        static void HandlePosition(const Arguments& args)
        {
            if (!UCIPrompted) return;

            uint8_t moveStrIndex = 2;
            if (strutil::compare_ignore_case(args[0], "fen")) {
                const Arguments   fenToken = {args.begin() + 1, args.begin() + 7};
                const std::string fen      = strutil::join(fenToken, " ");

                Board           = StockDory::Board(fen);
                Repetition      = {};
                HalfMoveCounter = std::stoi(fenToken[4]);

                Repetition.Push(Board.Zobrist());

                moveStrIndex = 8;
            } else if (strutil::compare_ignore_case(args[0], "startpos")) {
                Board      = {};
                Repetition = {};

                Repetition.Push(Board.Zobrist());
                HalfMoveCounter = 1;
            } else return;

            if (args.size() >= moveStrIndex &&
                strutil::compare_ignore_case(args[moveStrIndex - 1], "moves"))
                for (const Arguments movesToken = {args.begin() + moveStrIndex, args.end()};
                     const std::string& moveStr: movesToken) {
                    const Move move = Move::FromString(moveStr);

                    if (Board[move.To()].Piece() != NAP || Board[move.From()].Piece() == Pawn) HalfMoveCounter = 1;
                    else                                                                       HalfMoveCounter++;

                    Board.Move<ZOBRIST>(move.From(), move.To(), move.Promotion());

                    Repetition.Push(Board.Zobrist());
                }
        }

        template<typename T>
        [[nodiscard]]
        // ReSharper disable once CppDFAConstantParameter
        static T TokenToValue(const Arguments& args, const std::string& token, const T defaultValue)
        {
            for (size_t i = 0; i < args.size(); i++)
                if (strutil::compare_ignore_case(args[i], token) && i + 1 < args.size())
                    return static_cast<T>(std::stoull(args[i + 1]));

            return defaultValue;
        }

        static void HandleGo(const Arguments& args)
        {
            if (!UCIPrompted || Search.Running()) return;

            if (args.size() > 1 && strutil::compare_ignore_case(args[0], "perft")) {
                const auto depth = static_cast<uint8_t>(std::stoull(args[1]));
                PerftRunner::SetBoard(Board);
                PerftRunner::Perft<true>(depth);
                return;
            }

            Limit limit;

            if (args.size() == 2) {
                if (strutil::compare_ignore_case(args[0], "depth"))
                    limit.Depth = static_cast<uint8_t>(std::stoull(args[1]));
                if (strutil::compare_ignore_case(args[0], "nodes"))
                    limit.Nodes =                      std::stoull(args[1]) ;
                if (strutil::compare_ignore_case(args[0], "movetime")) {
                    const UCITime<true> time { .Time = std::stoull(args[1]) };
                    time.AsLimit(limit);
                }
            } else if (args.size() > 2) {
                const UCITime<false> time {
                    .WhiteTime = TokenToValue<uint64_t>(args, "wtime"    , 0),
                    .BlackTime = TokenToValue<uint64_t>(args, "btime"    , 0),
                    .WhiteInc  = TokenToValue<uint64_t>(args, "winc"     , 0),
                    .BlackInc  = TokenToValue<uint64_t>(args, "binc"     , 0),
                    .MovesToGo = TokenToValue<uint16_t>(args, "movestogo", 0)
                };

                time.AsLimit(limit);
            }

            Search.Run(limit, Board, Repetition, HalfMoveCounter);
        }

        static void HandleStop()
        {
            if (!UCIPrompted) return;

            Search.Stop();
        }

    };

} // StockDory

StockDory::UCIInterface::UCISearch StockDory::UCIInterface::Search = UCISearch(SearchEventHandler);

#endif //STOCKDORY_UCIINTERFACE_H
