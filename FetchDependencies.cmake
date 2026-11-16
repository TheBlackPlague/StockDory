include(DownloadCPM.cmake)

CPMAddPackage(
        NAME MantaRay
        GITHUB_REPOSITORY TheBlackPlague/MantaRay
        GIT_TAG 7d240a33868e751f4d97619d6903c1c0e8bdbad6
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
