set(INFORMATION_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/BuildSystem/Information.h")

set(INFORMATION_HEADER_CONTENT "
#ifndef INFORMATION_H
#define INFORMATION_H

#include <string>

const std::string NAME     = \"StockDory\"             \;
const std::string AUTHOR   = \"StockDory Contributors\"\;
const std::string VERSION  = \"${VERSION}\"            \;
const std::string CODENAME = \"${CODENAME}\"           \;
const std::string LICENSE  = \"LGPL-3.0\"              \;

#endif // INFORMATION_H
")

file(WRITE ${INFORMATION_HEADER_PATH} ${INFORMATION_HEADER_CONTENT})
