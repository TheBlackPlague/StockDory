include(DownloadCPM.cmake)

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