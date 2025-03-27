//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_THREADPOOL_H
#define STOCKDORY_THREADPOOL_H

#include <BS_thread_pool.hpp>

namespace StockDory
{

    inline BS::thread_pool<BS::tp::priority> ThreadPool;

} // StockDory

#endif //STOCKDORY_THREADPOOL_H
