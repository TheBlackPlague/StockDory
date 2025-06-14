cmake_minimum_required(VERSION 3.21)
project(StockDory)

set(VERSION  "0.1"     )
set(CODENAME "Starfish")

option(BUILD_NATIVE "Enable native CPU architecture optimizations" ON)
option(PYMODULE "Enable if building Python Bindings" OFF)
option(CI "Enable if building via Continuous Integration such as Github Actions" OFF)
option(PGO "Enable profile-guided optimization - uses existing profile or creates instrumentation executable" OFF)

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_CXX_FLAGS STREQUAL "")
    set(CMAKE_CXX_FLAGS "$ENV{CMAKE_CXX_FLAGS}")
endif()

if(BUILD_NATIVE AND NOT (PYMODULE OR CI))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
else()
    set(NANOTHREAD_NATIVE_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=full")

if(PGO)
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

CPMAddPackage(
        NAME MantaRay
        GITHUB_REPOSITORY TheBlackPlague/MantaRay
        GIT_TAG ddc063e1fa688ca3bc9de793e4d5a6813d901289
        OPTIONS
        "BUILD_TEST OFF"
        "BUILD_MB OFF"
)

CPMAddPackage(
        NAME nanothread
        GITHUB_REPOSITORY TheBlackPlague/nanothread
        GIT_TAG master
)

include(BinaryResource.cmake)

file(GLOB NNUEBinary
        "src/Engine/Model/*")

BinaryToHeader(${NNUEBinary} "${CMAKE_CURRENT_BINARY_DIR}/Model/NeuralNetworkBinary.h" "NeuralNetworkBinary")

include(Information.cmake)

Information(${VERSION} ${CODENAME} ${CI})

file(GLOB_RECURSE StockDoryBackendType "src/Backend/*.h")

file(GLOB_RECURSE StockDoryEngineType "src/Engine/*.h")

file(GLOB_RECURSE StockDoryExternalType "src/External/*.h")

file(GLOB_RECURSE StockDoryFrontendType "src/Terminal/*.h")

if(PYMODULE)
    message(STATUS "Configured to build Python Module")

    if(UNIX AND NOT APPLE)
        message(STATUS "Enabling Position-Independent Code")
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
        set_target_properties(nanothread PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()

    set(PYBIND11_FINDPYTHON ON)
    CPMAddPackage("gh:pybind/pybind11#a2e59f0e7065404b44dfe92a28aca47ba1378dc4")

    pybind11_add_module(StockDory src/python_binding.cpp
            ${StockDoryBackendType}
            ${StockDoryEngineType}
            ${StockDoryExternalType}
            ${StockDoryFrontendType}
    )

    target_include_directories(StockDory PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(StockDory PRIVATE MantaRay nanothread)
    if(NOT WIN32)
        target_link_libraries(StockDory PRIVATE pthread)
    endif()

    install(TARGETS StockDory DESTINATION .)
else()
    message(STATUS "Configured to build executable")

    add_executable(StockDory src/main.cpp
            ${StockDoryBackendType}
            ${StockDoryEngineType}
            ${StockDoryExternalType}
            ${StockDoryFrontendType}
    )

    target_include_directories(StockDory PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(StockDory MantaRay nanothread)
    if(NOT WIN32)
        target_link_libraries(StockDory pthread)
    endif()

    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
endif()

message(STATUS "NANOTHREAD COMPILATION: ${NANOTHREAD_NATIVE_FLAGS}")

message(STATUS "CXX         FLAGS: ${CMAKE_CXX_FLAGS}"        )
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "CXX DEBUG   FLAGS: ${CMAKE_CXX_FLAGS_DEBUG}"  )
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "CXX RELEASE FLAGS: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
