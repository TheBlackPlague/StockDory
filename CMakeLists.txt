cmake_minimum_required(VERSION 3.15)
project(StockDory)

set(VERSION  "0.1"     )
set(CODENAME "Starfish")

option(PYMODULE "Build Python Bindings for StockDory ... will not build an executable" OFF)

if(CI)
    set(CI TRUE)
else()
    set(CI FALSE)
endif()

set(CMAKE_CXX_STANDARD 23)
if(CMAKE_CXX_FLAGS STREQUAL "")
    set(CMAKE_CXX_FLAGS "-march=native")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=full")
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

set(CPM_DOWNLOAD_VERSION 0.40.8)

if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    message(STATUS "Setting CPM.cmake location to ${CPM_DOWNLOAD_LOCATION}")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    message(STATUS "Setting CPM.cmake location to ${CPM_DOWNLOAD_LOCATION}")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    message(STATUS "Setting CPM.cmake location to ${CPM_DOWNLOAD_LOCATION}")
endif()

set(CPM_DOWNLOAD_RETRY_COUNT 3)
foreach(DOWNLOAD_ATTEMPT RANGE 1 ${CPM_DOWNLOAD_RETRY_COUNT})
    if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
        message(STATUS "CPM.cmake was not found at ${CPM_DOWNLOAD_LOCATION}")
        message(STATUS "Attempt: ${DOWNLOAD_ATTEMPT}/${CPM_DOWNLOAD_RETRY_COUNT}: Downloading CPM.cmake...")
        file(DOWNLOAD
            https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
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

CPMAddPackage("gh:TheBlackPlague/MantaRay#526e4d9cedf13c8ffd1457cc293261d691cda6c4")

CPMAddPackage("gh:bshoshany/thread-pool@5.0.0")
add_library(BS_thread_pool INTERFACE)
target_include_directories(BS_thread_pool INTERFACE ${${CPM_LAST_PACKAGE_NAME}_SOURCE_DIR}/include)

CPMAddPackage(
        NAME nanothread
        GITHUB_REPOSITORY mitsuba-renderer/nanothread
        GIT_TAG master
        OPTIONS
            "NANOTHREAD_STATIC" ON
)

include(BinaryResource.cmake)

file(GLOB NNUEBinary
        "src/Engine/Model/*")

BinaryToHeader(${NNUEBinary} "${CMAKE_CURRENT_BINARY_DIR}/Model/NeuralNetworkBinary.h" "NeuralNetworkBinary")

include(Information.cmake)

Information(${VERSION} ${CODENAME} ${CI})

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

if(PYMODULE)
    message(STATUS "Configured to build Python Module")

    set(PYBIND11_FINDPYTHON ON)
    CPMAddPackage("gh:pybind/pybind11#a2e59f0e7065404b44dfe92a28aca47ba1378dc4")

    pybind11_add_module(StockDory src/Python/binding.cpp
            ${StockDoryBackendType}
            ${StockDoryEngineType}
            ${StockDoryExternalType}
            ${StockDoryFrontendType}
    )

    target_include_directories(StockDory PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(StockDory PRIVATE MantaRay BS_thread_pool nanothread)
    if(NOT WIN32)
        target_link_libraries(StockDory PRIVATE pthread)
    endif()
else()
    message(STATUS "Configured to build executable")

    add_executable(StockDory src/Terminal/main.cpp
            ${StockDoryBackendType}
            ${StockDoryEngineType}
            ${StockDoryExternalType}
            ${StockDoryFrontendType}
    )

    target_include_directories(StockDory PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(StockDory MantaRay BS_thread_pool nanothread)
    if(NOT WIN32)
        target_link_libraries(StockDory pthread)
    endif()
endif()

message(STATUS "CXX         FLAGS: ${CMAKE_CXX_FLAGS}"        )
message(STATUS "CXX DEBUG   FLAGS: ${CMAKE_CXX_FLAGS_DEBUG}"  )
message(STATUS "CXX RELEASE FLAGS: ${CMAKE_CXX_FLAGS_RELEASE}")
