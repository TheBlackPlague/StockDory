include(DownloadCPM.cmake)

CPMAddPackage(
        NAME MantaRay
        GITHUB_REPOSITORY TheBlackPlague/MantaRay
        GIT_TAG 44ff54318a67ef44c9cb68feb0736683b1c033ed
        OPTIONS
        "BUILD_TESTS OFF"
        "BUILD_BENCHMARKS OFF"
)

CPMAddPackage(
        NAME nanothread
        GITHUB_REPOSITORY TheBlackPlague/nanothread
        GIT_TAG master
)
