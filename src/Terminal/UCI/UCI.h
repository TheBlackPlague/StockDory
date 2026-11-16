//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_UCI_H
#define STOCKDORY_UCI_H

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <strutil.h>

#include "Information.h"

#include "Engine/Engine.h"

#include "Terminal/Output.h"

#include "UCISearchEventHandler.h"

namespace StockDory
{

    std::optional<int64_t> ParseInteger(const std::string_view text)
    {
        int64_t value = 0;

        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);

        if (error != std::errc() || end != text.data() + text.size()) return std::nullopt;

        return value;
    }

    class Arguments
    {

        std::vector<std::string> Tokens {};

        size_t Cursor = 0;

        public:
        explicit Arguments(const std::string& line) : Tokens(strutil::split_any(line, " \t\r\n"))
        {
            strutil::drop_empty(Tokens);
        }

        bool Done() const { return Cursor == Tokens.size(); }

        std::string_view Next() { return Done() ? std::string_view() : Tokens[Cursor++]; }

        bool Accept(const std::string_view keyword)
        {
            if (Done() || !strutil::compare_ignore_case(Tokens[Cursor], std::string(keyword))) return false;

            Cursor++;

            return true;
        }

        template<typename T>
        T Read(const T fallback)
        {
            static_assert(std::is_integral_v<T>, "Only whole numbers are read");

            const std::string_view token = Next();

            const bool             negative = token.starts_with('-');
            const std::string_view digits   = negative ? token.substr(1) : token;

            uint64_t value = 0;

            const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);

            if (error == std::errc::invalid_argument || end != digits.data() + digits.size()) return fallback;

            if (negative) return 0;

            constexpr auto Maximum = std::numeric_limits<T>::max();

            if (error == std::errc::result_out_of_range || value > static_cast<uint64_t>(Maximum)) return Maximum;

            return static_cast<T>(value);
        }

        std::string Until(const std::string_view keyword)
        {
            std::vector<std::string> taken;

            while (!Done() && !Accept(keyword)) taken.emplace_back(Next());

            return strutil::join(taken, " ");
        }

        std::string Rest() { return Until({}); }

    };

    struct UCIOption
    {

        std::string Name;
        std::string Declaration;

        std::function<void(std::string_view)> Set;

    };

    class UCI
    {

        using SpinSetter   = void (UCI::*)(int64_t);
        using CheckSetter  = void (UCI::*)(bool   );
        using ButtonAction = void (UCI::*)(       );

        constexpr static bool Always = false;

        struct Command
        {

            std::string_view Name;

            void (UCI::*Run)(Arguments);

            bool Handshake = true;

        };

        UCISearchEventHandler Handler {};

        std::vector<Command  > Commands {};
        std::vector<UCIOption> Options  {};

        bool Running     = true ;
        bool UCIPrompted = false;

        size_t Threads = 1;

        public:
        UCI()
        {
            Engine::Subscribe(Handler);

            Commands = {
                { .Name = "uci"       , .Run = &UCI::HandleUCI       , .Handshake = Always },
                { .Name = "isready"   , .Run = &UCI::HandleIsReady                         },
                { .Name = "setoption" , .Run = &UCI::HandleSetOption                       },
                { .Name = "ucinewgame", .Run = &UCI::HandleUCINewGame                      },
                { .Name = "position"  , .Run = &UCI::HandlePosition                        },
                { .Name = "go"        , .Run = &UCI::HandleGo                              },
                { .Name = "stop"      , .Run = &UCI::HandleStop                            },
                { .Name = "quit"      , .Run = &UCI::HandleQuit      , .Handshake = Always }
            };

            const int64_t threads  = static_cast<int64_t>(ThreadPool::HardwareLimit());

            Spin  ("Hash"         ,                     16, 1, 4194304, &UCI::SetHash        );
            Spin  ("Threads"      ,                      1, 1, threads, &UCI::SetThreads     );
            Spin  ("Move Overhead", TimeProcessingOverhead, 0,    5000, &UCI::SetMoveOverhead);
            Check ("UCI_ShowWDL"  , false                             , &UCI::SetShowWDL     );
            Check ("UCI_Chess960" , false                             , &UCI::SetChessFRC    );
            Button("Clear Hash"   ,                                     &UCI::ClearHash      );

            #ifdef BUILD_TUNING

            for (const TunableParameter& parameter : TunableParameter::All()) {
                const auto Set = [parameter](const int64_t value) -> void
                {
                    parameter.Set(static_cast<int32_t>(value));
                };

                Spin(parameter.Name, parameter.Default, parameter.Min, parameter.Max, Set);
            }

            #endif
        }

        UCI(const UCI&) = delete;

        UCI& operator =(const UCI&) = delete;

        void Launch()
        {
            Engine::NewGame();

            std::string line;

            while (Running && std::getline(std::cin, line)) HandleInput(line);

            Engine::Stop();
            Engine::Wait();
        }

        private:
        void Spin(
            const std::string_view                name,
            const int64_t                        value,
            const int64_t                      minimum,
            const int64_t                      maximum,
            const std::function<void(int64_t)>  setter
        )
        {
            Options.push_back({
                .Name        = std::string(name),
                .Declaration = std::format("type spin default {} min {} max {}", value, minimum, maximum),

                .Set = [=](const std::string_view text) -> void
                {
                    const std::optional<int64_t> parsed = ParseInteger(text);

                    if (!parsed || *parsed < minimum || *parsed > maximum) return Error("Option value out of range");

                    setter(*parsed);
                }
            });
        }

        void Spin(
            const std::string_view  name,
            const int64_t          value,
            const int64_t        minimum,
            const int64_t        maximum,
            const SpinSetter      setter
        )
        {
            const auto Apply = [this, setter](const int64_t parsed) -> void { (this->*setter)(parsed); };

            Spin(name, value, minimum, maximum, Apply);
        }

        void Check(const std::string_view name, const bool value, const CheckSetter setter)
        {
            Options.push_back({
                .Name        = std::string(name),
                .Declaration = std::format("type check default {}", value),

                .Set = [this, setter](const std::string_view text) -> void
                {
                    const std::string given (text);

                    if      (strutil::compare_ignore_case(given, "true" )) (this->*setter)(true );
                    else if (strutil::compare_ignore_case(given, "false")) (this->*setter)(false);
                    else                                                   Error("Option value must be true or false");
                }
            });
        }

        void Button(const std::string_view name, const ButtonAction action)
        {
            Options.push_back({
                .Name        = std::string(name),
                .Declaration = "type button",

                .Set = [this, action](const std::string_view) -> void { (this->*action)(); }
            });
        }

        // ReSharper disable CppMemberFunctionMayBeStatic
        void SetHash        (const int64_t value) { Engine::SetHash(static_cast<size_t>(value) * MB); }
        void SetMoveOverhead(const int64_t value) { Engine::SetMoveOverhead(MS(value));               }
        // ReSharper restore CppMemberFunctionMayBeStatic

        void SetShowWDL     (const bool    value) { Handler.ShowWDL  = value;                         }
        void SetChessFRC    (const bool    value) { Handler.ChessFRC = value;                         }

        void SetThreads(const int64_t value)
        {
            Threads = static_cast<size_t>(value);

            Engine::SetThreads(Threads);
        }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        void ClearHash() { Engine::ClearHash(); }

        static void Ignore(const std::string_view token)
        {
            Error(std::format("Unknown token ignored: {}", token));
        }

        void HandleInput(const std::string& line)
        {
            Arguments args (line);

            for (const auto& [name, run, handshake] : Commands) {
                if (!args.Accept(name)) continue;

                if (handshake && !UCIPrompted) return;

                return (this->*run)(args);
            }
        }

        // ReSharper disable once CppPassValueParameterByConstReference
        void HandleUCI(const Arguments _)
        {
            if (UCIPrompted) return;

            Print(std::format("id name {} {}", NAME, VERSION));
            Print(std::format("id nnue {}"   , NetworkName  ));
            Print(std::format("id author {}" , AUTHOR       ));
            Print(std::format("id license {}", LICENSE      ));

            for (const UCIOption& option : Options)
                Print(std::format("option name {} {}", option.Name, option.Declaration));

            Print("uciok");

            UCIPrompted = true;
        }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        // ReSharper disable once CppPassValueParameterByConstReference
        void HandleIsReady(const Arguments _) { Print("readyok"); }

        void HandleSetOption(Arguments args)
        {
            if (!args.Accept("name")) return Error("Expected setoption name <name> [value <value>]");

                  std::string name  = args.Until("value");
            const std::string value = args.Rest();

            if (strutil::compare_ignore_case(name, "WDL")) name = "UCI_ShowWDL";

            const auto Named = [&name](const UCIOption& option) -> bool
            {
                return strutil::compare_ignore_case(option.Name, name);
            };

            const auto option = std::ranges::find_if(Options, Named);

            if (option == Options.end()) return Error(std::format("Unknown option: {}", name));

            Engine::Stop();
            Engine::Wait();

            option->Set(value);
        }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        // ReSharper disable once CppPassValueParameterByConstReference
        void HandleUCINewGame(const Arguments _) { Engine::NewGame(); }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        void HandlePosition(Arguments args)
        {
            const bool startpos = args.Accept("startpos");

            if (!startpos && !args.Accept("fen")) return Error("Expected startpos or fen");

            const auto set = Engine::SetPosition(startpos ? StartFen : args.Until("moves"));

            if (!set) return Error(set.error());

            if (!startpos || args.Accept("moves")) {
                while (!args.Done()) {
                    const std::string_view text = args.Next();

                    const std::optional<Move> move = Engine::ParseMove(text);

                    if (!move) return Error(std::format("Illegal move: {}", text));

                    Engine::Play(*move);
                }
            }
        }

        void HandleGo(Arguments args)
        {
            if (args.Accept("perft")) return HandlePerft(args);

            SearchRequest request;

            while (!args.Done()) {
                if (args.Accept("infinite")) continue;

                     if (args.Accept("depth"    )) request.Depth            =    args.Read<uint8_t >(request.Depth) ;
                else if (args.Accept("nodes"    )) request.Nodes            =    args.Read<uint64_t>(request.Nodes) ;
                else if (args.Accept("movetime" )) request.MoveTime         = MS(args.Read<int64_t >(0            ));
                else if (args.Accept("wtime"    )) request.Time     [White] = MS(args.Read<int64_t >(0            ));
                else if (args.Accept("btime"    )) request.Time     [Black] = MS(args.Read<int64_t >(0            ));
                else if (args.Accept("winc"     )) request.Increment[White] = MS(args.Read<int64_t >(0            ));
                else if (args.Accept("binc"     )) request.Increment[Black] = MS(args.Read<int64_t >(0            ));
                else if (args.Accept("movestogo")) request.MovesToGo        =    args.Read<uint16_t>(0            ) ;

                else                               Ignore(args.Next());
            }

            Engine::Start(request);
        }

        static std::string Thousands(const uint64_t value)
        {
            std::string text = std::to_string(value);

            for (size_t i = text.size(); i > 3; i -= 3) text.insert(i - 3, ",");

            return text;
        }

        void HandlePerft(Arguments args)
        {
            const auto thousands = [](const uint64_t value) -> std::string
            {
                std::string text = std::to_string(value);

                for (size_t i = text.size(); i > 3; i -= 3) text.insert(i - 3, ",");

                return text;
            };

            const std::optional<int64_t> depth = ParseInteger(args.Next());

            if (!depth || *depth < 0 || *depth > std::numeric_limits<uint8_t>::max())
                return Error("Expected a perft depth from 0 to 255");

            PerftMode mode {};

            while (!args.Done()) {
                if (args.Accept("nobulk")) mode.Bulk = false;
                else                       Ignore(args.Next());
            }

            Engine::Stop();
            Engine::Wait();

            Print(std::format(
                "Running PERFT @ depth {} [Maximum Concurrency: {}t]{}:",
                *depth,
                Threads,
                mode.Bulk ? "" : " [nobulk]"
            ));

            const bool chessFRC = Handler.ChessFRC;

            const auto OnMove = [chessFRC](const Move move, const uint64_t nodes) -> void
            {
                Print(std::format("{}: {}", Engine::Board.ToString(move, chessFRC), nodes));
            };

            const auto [nodes, time] = Engine::Perft(static_cast<uint8_t>(*depth), mode, OnMove);

            const auto microseconds = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(time).count()
            );

            const double seconds = static_cast<double>(microseconds) / 1000000.0;
            const double elapsed = static_cast<double>(std::max<uint64_t>(microseconds, 1)) / 1000000.0;

            const auto nps = static_cast<uint64_t>(static_cast<double>(nodes) / elapsed);

            Print("");
            Print(std::format("Nodes searched: {}",  thousands(nodes)));
            Print(std::format("Time taken: {:g}s"  , seconds         ));
            Print(std::format("Speed: {} nps"      , thousands(nps)  ));
        }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        // ReSharper disable once CppPassValueParameterByConstReference
        void HandleStop(const Arguments _) { Engine::Stop(); }

        // ReSharper disable once CppPassValueParameterByConstReference
        void HandleQuit(const Arguments _) { Running = false; }

    };

} // StockDory

#endif //STOCKDORY_UCI_H
