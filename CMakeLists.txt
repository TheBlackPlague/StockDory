cmake_minimum_required(VERSION 3.25)
project(StockDory)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g -ftime-report")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mavx512f -mavx512bw")

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

include(FetchContent)

FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG v4.0.0
        )

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(MantaRay
        GIT_REPOSITORY https://github.com/TheBlackPlague/MantaRay
        GIT_TAG master
        )

FetchContent_MakeAvailable(MantaRay)

file(GLOB StockDoryBackendType
        "src/Backend/Type/*.h"
        "src/Backend/Template/*.h"
        "src/Backend/Move/*.h"
        "src/Backend/*.h")

file(GLOB StockDoryExternalType
        "src/External/*.h")

file(GLOB StockDoryFrontendType
        "src/Terminal/UI/*.h"
        "src/Terminal/Perft/*.h")

add_executable(StockDory src/Terminal/main.cpp
        ${StockDoryBackendType} ${StockDoryExternalType} ${StockDoryFrontendType})

target_link_libraries(StockDory ftxui::screen ftxui::dom ftxui::component MantaRay)
