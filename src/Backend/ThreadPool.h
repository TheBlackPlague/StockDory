//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_THREADPOOL_H
#define STOCKDORY_THREADPOOL_H

#include <nanothread/nanothread.h>

namespace StockDory
{

    inline Pool* ThreadPool = pool_create(core_count());

} // StockDory

#endif //STOCKDORY_THREADPOOL_H
