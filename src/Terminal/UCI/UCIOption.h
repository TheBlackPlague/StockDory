//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_UCIOPTION_H
#define STOCKDORY_UCIOPTION_H

#include <cstdint>
#include <string>
#include <sstream>
#include <functional>
#include <iostream>

namespace StockDory
{

    class UCIOptionBase
    {

        public:
            virtual ~UCIOptionBase() = default;

            [[nodiscard]]
            virtual std::string Log() const = 0;

            virtual void Set(const std::string& value) = 0;

    };

    template<typename T>
    class UCIOption : public UCIOptionBase
    {

        private:
            using Handler = std::function<void(const T&)>;

            std::string Name;

            T Default;

            typename std::enable_if<std::is_integral_v<T>, T>::type Min;
            typename std::enable_if<std::is_integral_v<T>, T>::type Max;

            Handler OptionHandler;

        public:
            UCIOption(const std::string& name, const T& defaultValue, const Handler& handler)
            {
                static_assert(std::is_integral<T>::value, "Min and Max must be defined for integral types.");

                Name = name;
                Default = defaultValue;
                OptionHandler = handler;
            }

            UCIOption(const std::string& name, const T& defaultValue, const T& min, const T& max, const Handler& handler)
            {
                static_assert(std::is_integral_v<T>, "Min and Max can only be used with integral types.");

                Name = name;
                Default = defaultValue;
                Min = min;
                Max = max;
                OptionHandler = handler;
            }

            [[nodiscard]]
            std::string GetName() const
            {
                return Name;
            }

            [[nodiscard]]
            std::string Log() const override
            {
                std::stringstream output;

                output << "option name " << Name << " ";

                if (std::is_integral_v<T>)
                    output << "type spin ";
                else if (std::is_same_v<T, bool>)
                    output << "type check ";
                else if (std::is_same_v<T, std::string>)
                    output << "type string ";
                else throw std::runtime_error("Unsupported type.");

                if      (std::is_same_v<T, uint8_t>)
                     output << "default " << static_cast<uint16_t>(Default);
                else if (std::is_same_v<T,  int8_t>)
                     output << "default " << static_cast< int16_t>(Default);
                else output << "default " << Default;

                if (std::is_integral_v<T>) {
                    if      (std::is_same_v<T, uint8_t>)
                         output << " min " << static_cast<uint16_t>(Min) << " max " << static_cast<uint16_t>(Max);
                    else if (std::is_same_v<T,  int8_t>)
                         output << " min " << static_cast< int16_t>(Min) << " max " << static_cast< int16_t>(Max);
                    else output << " min " << Min << " max " << Max;
                }

                return output.str();
            }

            void Set(const std::string& value) override
            {
                OptionHandler(strutil::parse_string<T>(value));
            }

    };

} // StockDory

#endif //STOCKDORY_UCIOPTION_H
