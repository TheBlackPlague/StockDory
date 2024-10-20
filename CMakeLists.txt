cmake_minimum_required(VERSION 3.29)
project(chessTest)

set(CMAKE_CXX_STANDARD 20)

add_executable(chessTest main.cpp
        Backend/Move/MoveList.h
        SimplifiedMoveList.h
)
