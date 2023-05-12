//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <iostream>

#include "../Backend/Information.h"

#include "UCI/UCIInterface.h"

int main()
{
    std::cout << Title << " " << Version << std::endl;
    std::cout << "Provided by " << Author << " under the " << License << " license." << std::endl;

    StockDory::UCIInterface::Launch();

    return 0;
}
