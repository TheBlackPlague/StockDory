//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_THREADPOOL_H
#define STOCKDORY_THREADPOOL_H

#include <thread>

#include <nanothread/nanothread.h>

class ThreadPool
{

    public:
    static size_t HardwareLimit()
    {
        return core_count();
    }

    private:
    Pool* Internal = nullptr;

    public:
    ThreadPool(const size_t n) { Internal = pool_create(n); }

    ~ThreadPool() { if (Internal != nullptr) pool_destroy(Internal); }

    size_t Size() const { return pool_size(Internal); }

    void Resize(const size_t n)
    {
        if (Internal != nullptr) pool_destroy(Internal);
        Internal = pool_create(n);
    }

    template<typename F>
    void Execute(F&& code)
    {
        Task* task = drjit::do_async(code, {}, Internal);
        task_release(task);
    }

    template<typename T, typename F>
    void For(const drjit::blocked_range<T>& range, F&& code)
    {
        drjit::parallel_for(
            range,
            std::forward<F>(code),
            Internal
        );
    }

};

namespace StockDory
{

    void Sleep(const uint64_t ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    [[clang::always_inline]]
    size_t CurrentThreadID()
    {
        return pool_thread_id();
    }

    inline ThreadPool ThreadPool (1);

} // StockDory

#endif //STOCKDORY_THREADPOOL_H
