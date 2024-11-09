cmake_minimum_required(VERSION 3.26)
project(MulticoreChess)

set(CMAKE_CXX_STANDARD 20)

add_executable(MulticoreChess main.cpp
        Backend/Move/MoveList.h
        SimplifiedMoveList.h
        Evaluation.h
        Engine.h
)

find_package(OpenMP REQUIRED)
if (OpenMP_C_FOUND)
    target_link_options(MulticoreChess PUBLIC -fopenmp)
    target_link_options(MulticoreChess PUBLIC -fopenmp)

endif()