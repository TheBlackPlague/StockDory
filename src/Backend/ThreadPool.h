//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_THREADPOOL_H
#define STOCKDORY_THREADPOOL_H

#include <nanothread/nanothread.h>

class ThreadPool
{

    public:
    static inline size_t HardwareLimit()
    {
        return core_count();
    }

    private:
    Pool* Internal = nullptr;

    public:
    ThreadPool(const size_t n) { Internal = pool_create(n); }

    ~ThreadPool() { if (Internal != nullptr) pool_destroy(Internal); }

    inline size_t Size() const { return pool_size(Internal); }

    inline void Resize(const size_t n)
    {
        if (Internal != nullptr) pool_destroy(Internal);
        Internal = pool_create(n);
    }

    inline Pool* operator ~() const { return Internal; }

    template<typename F>
    inline void Execute(F&& code)
    {
        Task* task = drjit::do_async(code, {}, Internal);
        task_release(task);
    }

};

namespace StockDory
{

    inline ThreadPool ThreadPool (1);

} // StockDory

#endif //STOCKDORY_THREADPOOL_H
