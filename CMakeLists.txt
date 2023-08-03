cmake_minimum_required(VERSION 3.15)
project(StockDory)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-march=native -flto")

if(PGO MATCHES "True")
    message(STATUS "PGO: Enabled")
    if(EXISTS ${CMAKE_BINARY_DIR}/pgo.profdata)
        message(STATUS "PGO: Using existing profile data at: ${CMAKE_BINARY_DIR}/pgo.profdata")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use=${CMAKE_BINARY_DIR}/pgo.profdata")
    else()
        message(STATUS "PGO: No existing profile data found. Compiling with instrumentation.")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate=${CMAKE_BINARY_DIR}/pgo.profraw")
    endif()
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g -ftime-report")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

set(CPM_DOWNLOAD_VERSION 0.38.0)

if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    message(STATUS "Setting CPM.cmake location to ${CPM_DOWNLOAD_LOCATION}")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    message(STATUS "Setting CPM.cmake location to ${CPM_DOWNLOAD_LOCATION}")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    message(STATUS "Setting CPM.cmake location to ${CPM_DOWNLOAD_LOCATION}")
endif()

set(CPM_DOWNLOAD_RETRY_COUNT 3)
foreach(DOWNLOAD_ATTEMPT RANGE 1 ${CPM_DOWNLOAD_RETRY_COUNT})
    if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
        message(STATUS "CPM.cmake was not found at ${CPM_DOWNLOAD_LOCATION}")
        message(STATUS "Attempt: ${DOWNLOAD_ATTEMPT}/${CPM_DOWNLOAD_RETRY_COUNT}: Downloading CPM.cmake...")
        file(DOWNLOAD
             https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
             ${CPM_DOWNLOAD_LOCATION}
             STATUS DOWNLOAD_STAT
        )
        list(GET DOWNLOAD_STAT 0 DOWNLOAD_RES)
        if(NOT DOWNLOAD_RES EQUAL 0)
            if (DOWNLOAD_ATTEMPT EQUAL CPM_DOWNLOAD_RETRY_COUNT)
                message(FATAL_ERROR "Max download attempts exceeded.")
                break()
            endif()

            message(STATUS "Download failed. Removing ${CPM_DOWNLOAD_LOCATION} and retrying in 5 seconds...")
            file(REMOVE ${CPM_DOWNLOAD_LOCATION})
            execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 5)
        endif()
    endif()

    if(EXISTS ${CPM_DOWNLOAD_LOCATION})
        message(STATUS "CPM.cmake was found at ${CPM_DOWNLOAD_LOCATION}")
        break()
    endif()
endforeach()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage("gh:TheBlackPlague/MantaRay#5299e77043cf7b0b7cab5c79ac7d5769a5f9474d")

include(BinaryResource.cmake)

file(GLOB NNUEBinary
        "src/Engine/Model/*")

BinaryToHeader(${NNUEBinary} "${CMAKE_CURRENT_BINARY_DIR}/Model/NeuralNetworkBinary.h" "NeuralNetworkBinary")

file(GLOB StockDoryBackendType
        "src/Backend/Type/*.h"
        "src/Backend/Template/*.h"
        "src/Backend/Move/*.h"
        "src/Backend/*.h")

file(GLOB StockDoryEngineType
        "src/Engine/Move/*.h"
        "src/Engine/Time/*.h"
        "src/Engine/*.h")

file(GLOB StockDoryExternalType
        "src/External/*.h"
        "src/External/thread_pool.hpp")

file(GLOB StockDoryFrontendType
        "src/Terminal/UI/*.h"
        "src/Terminal/Perft/*.h"
        "src/Terminal/UCI/*.h"
        "src/Terminal/*.h")

add_executable(StockDory src/Terminal/main.cpp
        ${StockDoryBackendType} ${StockDoryEngineType} ${StockDoryExternalType} ${StockDoryFrontendType})

target_include_directories(StockDory PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(StockDory MantaRay)

if(NOT WIN32)
    target_link_libraries(StockDory pthread)
endif()