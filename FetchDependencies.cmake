include(DownloadCPM.cmake)

CPMAddPackage(
        NAME MantaRay
        GITHUB_REPOSITORY TheBlackPlague/MantaRay
        GIT_TAG 344d7225885e5a82a12bf54a34fdc679d96d9831
        OPTIONS
        "BUILD_EXAMPLES OFF"
)

CPMAddPackage(
        NAME fastrange
        GITHUB_REPOSITORY lemire/fastrange
        GIT_TAG f6e712b4725fc9443a11516a6302f5220e0ed951
        DOWNLOAD_ONLY YES
)

CPMAddPackage(
        NAME strutil
        GITHUB_REPOSITORY Shot511/strutil
        GIT_TAG dafb13e8e5e7761dc9a731862a666221e82cd546
        DOWNLOAD_ONLY YES
)
