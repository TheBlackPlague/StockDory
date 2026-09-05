//
// Copyright (c) 2023-2026 Shaheryar Sohail
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_THREADPOOL_H
#define STOCKDORY_THREADPOOL_H

#include <thread>

#include <nanothread/nanothread.h>

using Block = drjit::blocked_range<uint8_t>;

class ThreadedTask
{

    Task* Internal = nullptr;

    public:
    ThreadedTask() = default;

    explicit ThreadedTask(Task* task) noexcept : Internal(task) {}

    ~ThreadedTask() { Reset(); }

    ThreadedTask(const ThreadedTask&) = delete;
    ThreadedTask& operator =(const ThreadedTask&) = delete;

    ThreadedTask(ThreadedTask&& other) noexcept : Internal(std::exchange(other.Internal, nullptr)) {}

    ThreadedTask& operator =(ThreadedTask&& other) noexcept
    {
        if (this != &other) {
            Reset();
            Internal = std::exchange(other.Internal, nullptr);
        }

        return *this;
    }

    [[nodiscard]]
    Task* Get() const noexcept { return Internal; }

    [[nodiscard]]
    explicit operator bool() const noexcept { return Internal != nullptr; }

    void Wait() const { if (Internal != nullptr) task_wait(Internal); }

    void Reset() noexcept
    {
        if (Internal == nullptr) return;

        task_release(std::exchange(Internal, nullptr));
    }

};

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
    ThreadedTask Execute(F&& code)
    {
        return ThreadedTask(drjit::do_async(std::forward<F>(code), {}, Internal));
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
