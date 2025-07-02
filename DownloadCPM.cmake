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
    if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
        message(STATUS "CPM.cmake was not found at ${CPM_DOWNLOAD_LOCATION}")
        message(STATUS "Attempt: ${DOWNLOAD_ATTEMPT}/${CPM_DOWNLOAD_RETRY_COUNT}: Downloading CPM.cmake...")
        file(DOWNLOAD
                https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
                ${CPM_DOWNLOAD_LOCATION}
                STATUS DOWNLOAD_STAT
        )
        list(GET DOWNLOAD_STAT 0 DOWNLOAD_RES)
        if(NOT DOWNLOAD_RES EQUAL 0)
            if(DOWNLOAD_ATTEMPT EQUAL CPM_DOWNLOAD_RETRY_COUNT)
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