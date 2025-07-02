cmake_minimum_required(VERSION 3.21)

project(StockDory)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS         "-mno-avx512vnni"               )
set(CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wextra -g -ftime-report")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG"                  )

set(VERSION  "0.2"    )
set(CODENAME "Piranha")

option(BUILD_NATIVE     "Build with native optimizations enabled"          ON )
option(BUILD_PGO        "Build with Profile Guided Optimization"           OFF)
option(BUILD_CI         "Build for CI - BUILD_NATIVE/BUILD_PGO is ignored" OFF)
option(BUILD_PRODUCTION "Build for production - Version is solidified"     OFF)

if (NOT BUILD_PRODUCTION)
    execute_process(
            COMMAND git rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_FULL_SHA
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE GIT_REV_PARSE_RESULT
    )

    if (NOT GIT_REV_PARSE_RESULT EQUAL 0)
        message(WARNING "Could not get hash. Is this a Git repository?")
        set(GIT_SHORT_SHA "unknown")
    else()
        string(SUBSTRING "${GIT_FULL_SHA}" 0 10 GIT_SHORT_SHA)
    endif()

    set(VERSION ${GIT_SHORT_SHA})
endif ()

message(STATUS "Building StockDory ${VERSION} (${CODENAME})")

include(Information.cmake)

if (BUILD_CI)
    set(BUILD_NATIVE OFF)
    set(BUILD_PGO    OFF)
endif ()

if (BUILD_NATIVE)
    set(ARCHITECTURE "native")
endif ()

if (ARCHITECTURE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${ARCHITECTURE} -mtune=${ARCHITECTURE}")
endif ()

if (BUILD_PGO)
    if (NOT EXISTS ${CMAKE_BINARY_DIR}/pgo.profdata)
        message(STATUS "PGO: No profile data found, adding profiling steps to the build...")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate=${CMAKE_BINARY_DIR}/pgo.profraw")
    else ()
        message(STATUS "PGO: Profile data found, using it for optimization...")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use=${CMAKE_BINARY_DIR}/pgo.profdata")
    endif ()
endif ()

include(FetchDependencies.cmake)
include(EmbedFile.cmake)

file(GLOB NNModel "src/Engine/Model/*.nnue")

if (NNModel)
    message(STATUS "Embedding NNUE Model: ${NNModel}")
    EmbedFile(
            ${NNModel}
            ${CMAKE_CURRENT_BINARY_DIR}/BuildSystem/Model/NeuralNetworkBinary.h
            NeuralNetworkBinary
    )
endif ()

file(GLOB_RECURSE HEADER "src/*.h")

add_executable(StockDory src/main.cpp ${HEADER})

target_include_directories(StockDory PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/BuildSystem)
target_link_libraries(StockDory MantaRay nanothread)

if (NOT WIN32)
    target_link_libraries(StockDory pthread)
endif ()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "Flags: ${CMAKE_CXX_FLAGS} | ${CMAKE_CXX_FLAGS_RELEASE}")
elseif (CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "Flags: ${CMAKE_CXX_FLAGS} | ${CMAKE_CXX_FLAGS_DEBUG}")
else ()
    message(STATUS "Flags: ${CMAKE_CXX_FLAGS}")
endif ()