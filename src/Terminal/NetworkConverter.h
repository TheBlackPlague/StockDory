//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_NETWORKCONVERTER_H
#define STOCKDORY_NETWORKCONVERTER_H

#include <string>

#include "../Engine/NetworkArchitecture.h"

#include "../External/emoji.h"
#include "../External/picosha2.h"
#include "../External/strutil.h"

namespace StockDory
{

    class NetworkConverter
    {

        constexpr static char Separator =
#ifdef _WIN32
            '\\';
#else
            '/' ;
#endif

        static std::string JoinPath(const std::vector<std::string>& paths)
        {
            std::stringstream ss;

            uint8_t i = 0;
            for (const std::string& path: paths) {
                ss << path;
                if (i != paths.size() - 1) ss << Separator;
                i++;
            }

            return ss.str();
        }

        template<typename T>
        static std::shared_ptr<T> ReadFromFile(const std::string& path)
        {
            std::cout << emojicpp::emojize(":brain: Reading network from file... ");
            MantaRay::MarlinflowStream stream (path);
            std::cout << emojicpp::emojize(":white_check_mark:") << std::endl;
            return std::make_shared<T>(stream);
        }

        template<typename T>
        static void WriteToFile(const std::string& directory, const std::shared_ptr<T>& network)
        {
            const std::string          path = JoinPath({directory, "temp.nnue"});
            MantaRay::BinaryFileStream stream (path);
            network->WriteTo(stream);
        }

        static void GenerateHash(const std::string& directory, const std::string& architecture)
        {
            const std::string path = JoinPath({directory, "temp.nnue"});

            std::cout << emojicpp::emojize(":magic_wand: Generating network hash... ");
            std::ifstream              hashStream (path, std::ios::binary);
            std::vector<unsigned char> hashBlock  (picosha2::k_digest_size);
            picosha2::hash256(hashStream, hashBlock.begin(), hashBlock.end());
            hashStream.close();
            std::string hash = picosha2::bytes_to_hex_string(hashBlock.begin(), hashBlock.end())
                    .substr(0, 10);
            std::cout << emojicpp::emojize(":white_check_mark:") << std::endl;

            std::cout << emojicpp::emojize(":floppy_disk: Saving network... ");
            const std::string arch    = strutil::capitalize_first_char(strutil::to_lower(architecture));
            const std::string name    = arch + "-" + hash + ".nnue";
            const std::string newPath = JoinPath({directory, name});
            std::rename(path.c_str(), newPath.c_str());
            std::cout << emojicpp::emojize(":white_check_mark: - Network saved as: " + name) << std::endl;
        }

        public:
        static void Launch()
        {
            std::cout << emojicpp::emojize(":dna: Neural Network Converter :dna:") << std::endl;

            std::cout << emojicpp::emojize(":open_folder: JSON Path: ");
            std::string inputPath;
            std::getline(std::cin, inputPath);

            std::cout << emojicpp::emojize(":open_folder: Architecture: ");
            std::string architecture;
            std::getline(std::cin, architecture);

            std::cout << emojicpp::emojize(":open_folder: Output Directory: ");
            std::string outputDirectory;
            std::getline(std::cin, outputDirectory);

            if (strutil::compare_ignore_case(architecture, "Starshard")) {
                const std::shared_ptr<Starshard> network = ReadFromFile<Starshard>(inputPath);
                WriteToFile<Starshard>(outputDirectory, network);
            } else if (strutil::compare_ignore_case(architecture, "Aurora")) {
                const std::shared_ptr<Aurora   > network = ReadFromFile<Aurora   >(inputPath);
                WriteToFile<Aurora>(outputDirectory, network);
            }

            GenerateHash(outputDirectory, architecture);
        }

    };

} // StockDory

#endif //STOCKDORY_NETWORKCONVERTER_H
