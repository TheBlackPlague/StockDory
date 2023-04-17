#include <iostream>

#include "../Backend/Type/BitBoard.h"

#include "../Backend/Move/UtilityTable.h"
#include "../Backend/Move/TableSetup.h"

int main()
{
    TableSetup();

    BitBoard x = BBDefault;
    x |= StockDory::UtilityTable::Between[Square::A1][Square::H8];
    x |= StockDory::UtilityTable::Between[Square::H1][Square::H8];
    x |= StockDory::UtilityTable::Between[Square::B3][Square::H3];

    std::cout << ToString(x) << std::endl;
    return 0;
}
