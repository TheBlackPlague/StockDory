cmake_minimum_required(VERSION 3.25)
project(StockDory)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g -ftime-report")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

set(CPM_DOWNLOAD_VERSION 0.38.0)

if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
            https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
            ${CPM_DOWNLOAD_LOCATION}
            )
endif()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage("gh:ArthurSonzogni/ftxui#v4.0.0")
CPMAddPackage("gh:TheBlackPlague/MantaRay#master")

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
