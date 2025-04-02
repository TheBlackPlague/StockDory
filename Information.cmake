set(INFORMATION_HEADER_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/Information.h")

function(Information VERSION CODENAME DEV)
    if (DEV)
        string(TIMESTAMP DATE "%m%d%Y")
        set(VERSION "${VERSION}-${DATE}")
    endif()

    message(STATUS "Generating Information.h")

    message(STATUS "Version : ${VERSION}" )
    message(STATUS "Codename: ${CODENAME}")

    set(HEADER_CONTENT "
    #ifndef INFORMATION_H
    #define INFORMATION_H

    #include <string>

    const std::string NAME     = \"StockDory\"             \;
    const std::string AUTHOR   = \"the StockDory Authors\" \;
    const std::string VERSION  = \"${CODENAME}-${VERSION}\"\;
    const std::string LICENSE  = \"LGPL-3.0\"              \;

    #endif // INFORMATION_H
    ")

    file(WRITE ${INFORMATION_HEADER_LOCATION} ${HEADER_CONTENT})
endfunction()