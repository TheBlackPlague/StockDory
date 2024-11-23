#cmake_minimum_required(VERSION 3.26)
#project(MulticoreChess)
#
## Set C++ standard
#set(CMAKE_CXX_STANDARD 20)
#
## Enable debug symbols
#set(CMAKE_BUILD_TYPE Debug)
#
## Add executable target
#add_executable(MulticoreChess
#        main.cpp
#        Backend/Move/MoveList.h
#        SimplifiedMoveList.h
#        Evaluation.h
#        Engine.h
#)
#
## Find and enable OpenMP
#find_package(OpenMP REQUIRED)
#if (OpenMP_C_FOUND)
#    target_compile_options(MulticoreChess PUBLIC -fopenmp)
#    target_link_options(MulticoreChess PUBLIC -fopenmp)
#endif()
#
## Add additional debugging options for GCC/Clang
#if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#    target_compile_options(MulticoreChess PUBLIC -Wall -Wextra -g -O0)
#endif()

cmake_minimum_required(VERSION 3.26)
project(MulticoreChess)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Debug)



add_executable(MulticoreChess main.cpp
        Backend/Move/MoveList.h
        SimplifiedMoveList.h
        Evaluation.h
        Engine.h
)
add_executable(play-bot play-bot.cpp
        Backend/Move/MoveList.h
        SimplifiedMoveList.h
        Evaluation.h
        Engine.h
)
add_executable(m4 m4.cpp
        Backend/Move/MoveList.h
        SimplifiedMoveList.h
        Evaluation.h
        Engine.h
)

find_package(OpenMP REQUIRED)
if (OpenMP_C_FOUND)
    target_compile_options(MulticoreChess PUBLIC -fopenmp)
    target_link_options(MulticoreChess PUBLIC -fopenmp)
    target_compile_options(play-bot PUBLIC -fopenmp)
    target_link_options(play-bot PUBLIC -fopenmp)
    target_compile_options(m4 PUBLIC -fopenmp)
    target_link_options(m4 PUBLIC -fopenmp)
endif()