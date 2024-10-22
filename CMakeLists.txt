cmake_minimum_required(VERSION 3.29)
project(MulticoreChess)

set(CMAKE_CXX_STANDARD 20)

add_executable(MulticoreChess main.cpp
        Backend/Move/MoveList.h
        SimplifiedMoveList.h
        Evaluation.h
        Engine.h
)
