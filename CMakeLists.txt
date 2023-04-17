cmake_minimum_required(VERSION 3.25)
project(StockDory)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -fconstexpr-steps=100000000")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ftime-report")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mavx512f -mavx512bw")

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

file(GLOB MantaRaySRC
        "src/External/MantaRay/src/Activation/*.h"
        "src/External/MantaRay/src/Backend/*.h"
        "src/External/MantaRay/src/External/*.hpp"
        "src/External/MantaRay/src/IO/*.h"
        "src/External/MantaRay/src/Perspective/*.h"
        "src/External/MantaRay/src/AccumulatorOperation.h"
        "src/External/MantaRay/src/SIMD.h")

add_library(MantaRay ${MantaRaySRC})

set_target_properties(MantaRay PROPERTIES LINKER_LANGUAGE CXX)

file(GLOB StockDoryBackendType
        "src/Backend/Type/*.h"
        "src/Backend/Template/*.h"
        "src/Backend/Move/*.h"
        "src/Backend/*.h")

add_executable(StockDory src/Terminal/main.cpp ${StockDoryBackendType})
