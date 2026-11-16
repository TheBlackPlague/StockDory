//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_THREADPOOL_H
#define STOCKDORY_THREADPOOL_H

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#else

#include <pthread.h>

#endif

#ifdef __linux__

#include <cerrno>
#include <fstream>
#include <string>

#include <sched.h>

#endif

#ifdef __APPLE__

#include <sys/sysctl.h>
#include <sys/types.h>

#endif

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "Common.h"

namespace StockDory
{

    class ThreadPool
    {

        public:
        using Job = std::function<void(size_t)>;

        private:
        class NativeThread
        {

            using Body = std::function<void()>;

            constexpr static size_t StackSize = 8 * MB;

            #ifdef _WIN32

            HANDLE Internal = nullptr;

            static DWORD WINAPI Entry(void* body)
            {
                const std::unique_ptr<Body> owned (static_cast<Body*>(body));

                (*owned)();

                return 0;
            }

            #else

            pthread_t Internal {};

            static void* Entry(void* body)
            {
                const std::unique_ptr<Body> owned (static_cast<Body*>(body));

                (*owned)();

                return nullptr;
            }

            #endif

            public:
            explicit NativeThread(Body body)
            {
                const auto owned = new Body(std::move(body));

                #ifdef _WIN32

                Internal = CreateThread(nullptr, StackSize, Entry, owned, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);

                if (Internal == nullptr) UnrecoverableError();

                #else

                pthread_attr_t attributes;

                if (pthread_attr_init        (&attributes)                          != 0 ||
                    pthread_attr_setstacksize(&attributes, StackSize)               != 0 ||
                    pthread_create           (&Internal, &attributes, Entry, owned) != 0  ) UnrecoverableError();

                pthread_attr_destroy(&attributes);

                #endif
            }

            void Join()
            {
                #ifdef _WIN32

                WaitForSingleObject(Internal, INFINITE);
                CloseHandle        (Internal);

                #else

                pthread_join(Internal, nullptr);

                #endif
            }

        };

        struct Loop
        {

            void (*Call)(void*, size_t) = nullptr;
            void*  Body                 = nullptr;

            size_t Count = 0;
            size_t Next  = 0;
            size_t Done  = 0;

            Loop* Older = nullptr;
            Loop* Newer = nullptr;

        };

        std::vector<NativeThread> Threads {};

        Job Launched {};

        alignas(DestructiveAlignment) Atomic<uint32_t> Signal   { 0 };
                                      Atomic<uint32_t> Sleepers { 0 };

        Atomic<uint32_t> Launches { 0 };
        Atomic<uint32_t> Running  { 0 };

        Atomic<bool> Closing { false };

        std::mutex Guard {};

        Loop* Oldest = nullptr;
        Loop* Newest = nullptr;

        void Notify()
        {
            Signal.Add(1);

            if (Sleepers.Load() != 0) Signal.NotifyAll();
        }

        void Sleep(const uint32_t signal)
        {
            Sleepers.Add(1);

            if (Signal.Load() == signal) Signal.Wait(signal);

            Sleepers.Subtract(1);
        }

        void Open(Loop& loop)
        {
            {
                const std::lock_guard lock (Guard);

                loop.Older = Newest;

                (Newest != nullptr ? Newest->Newer : Oldest) = &loop;

                Newest = &loop;
            }

            Notify();
        }

        size_t Take(Loop& loop)
        {
            const size_t item = loop.Next++;

            if (loop.Next == loop.Count) {
                (loop.Older != nullptr ? loop.Older->Newer : Oldest) = loop.Newer;
                (loop.Newer != nullptr ? loop.Newer->Older : Newest) = loop.Older;
            }

            return item;
        }

        bool Help()
        {
            Loop*  loop;
            size_t item;

            {
                const std::lock_guard lock (Guard);

                loop = Oldest;

                if (loop == nullptr) return false;

                item = Take(*loop);
            }

            loop->Call(loop->Body, item);

            bool last;

            {
                const std::lock_guard lock (Guard);

                last = ++loop->Done == loop->Count;
            }

            if (last) Notify();

            return true;
        }

        void Work(const size_t index, uint32_t seen)
        {
            while (true) {
                const uint32_t signal = Signal.Load();

                if (Closing.Load()) return;

                if (const uint32_t launches = Launches.Load(); launches != seen) {
                    seen = launches;

                    Launched(index);

                    if (Running.Subtract(1) == 0) Notify();

                    continue;
                }

                if (Help()) continue;

                Sleep(signal);
            }
        }

        public:
        explicit ThreadPool(const size_t size) { Resize(size); }

        ~ThreadPool() { Resize(0); }

        static size_t HardwareLimit();

        size_t Size() const { return Threads.size(); }

        void Resize(const size_t size)
        {
            assert(!Busy());

            Closing.Store(true);

            Notify();

            for (NativeThread& thread : Threads) thread.Join();

            Threads.clear();

            Closing.Store(false);

            const uint32_t seen = Launches.Load();

            for (size_t index = 0; index < size; index++)
                Threads.emplace_back([this, index, seen] -> void { Work(index, seen); });
        }

        void Launch(Job job)
        {
            assert(!Busy());

            Launched = std::move(job);

            Running.Store(static_cast<uint32_t>(Size()));
            Launches.Add(1);

            Notify();
        }

        void Wait()
        {
            while (true) {
                const uint32_t signal = Signal.Load();

                if (!Busy()) return;

                Sleep(signal);
            }
        }

        bool Busy() const { return Running.Load() != 0; }

        void Run(Job job)
        {
            Launch(std::move(job));

            Wait();
        }

        template<typename F>
        void For(const size_t count, F&& body)
        {
            if (count == 0) return;

            using Function = std::remove_reference_t<F>;

            Loop loop
            {
                .Call  = [](void* f, const size_t item) -> void { (*static_cast<Function*>(f))(item); },
                .Body  = const_cast<std::remove_const_t<Function>*>(&body),
                .Count = count
            };

            Open(loop);

            while (true) {
                size_t item;

                {
                    const std::lock_guard lock (Guard);

                    if (loop.Next == loop.Count) break;

                    item = Take(loop);
                }

                body(item);

                const std::lock_guard lock (Guard);

                loop.Done++;
            }

            while (true) {
                const uint32_t signal = Signal.Load();

                {
                    const std::lock_guard lock (Guard);

                    if (loop.Done == loop.Count) break;
                }

                if (Help()) continue;

                Sleep(signal);
            }
        }

    };

    size_t ThreadPool::HardwareLimit()
    {
        static const size_t limit = [] -> size_t
        {
            size_t count = 0;
            size_t quota = SIZE_MAX;

            #if defined(__linux__)

            for (size_t cpus = 1024; count == 0 && cpus <= (static_cast<size_t>(1) << 20); cpus *= 2) {
                std::vector<unsigned long> mask (cpus / (8 * sizeof(unsigned long)));

                cpu_set_t* const set = reinterpret_cast<cpu_set_t*>(mask.data());

                if (sched_getaffinity(0, mask.size() * sizeof(unsigned long), set) == 0)
                    for (const unsigned long word : mask) count += static_cast<size_t>(std::popcount(word));
                else if (errno != EINVAL) break;
            }

            const std::string procCgroup = "/proc/self/cgroup", cgroupRoot = "/sys/fs/cgroup";
            const auto Tighten = [&](const std::string& directory, const bool v2) -> void
            {
                long long q = 0, p = 0;

                std::ifstream first  (directory + (v2 ? "/cpu.max" : "/cpu.cfs_quota_us" ));
                std::ifstream period (directory +                    "/cpu.cfs_period_us" );

                if ((v2 ? !!(first >> q >> p) : (first >> q && period >> p)) && q > 0 && p > 0)
                    quota = std::min(quota, static_cast<size_t>(q / p));
            };

            std::ifstream groups (procCgroup);
            for (std::string line; std::getline(groups, line);) {
                const size_t first  = line.find(':');
                const size_t second = first == std::string::npos ? first : line.find(':', first + 1);

                if (second == std::string::npos || line.compare(second + 1, 1, "/") != 0) continue;

                const std::string controllers = "," + line.substr(first + 1, second - first - 1) + ",";

                const bool v2 = controllers == ",,";

                if (!v2 && controllers.find(",cpu,") == std::string::npos) continue;

                for (std::string path = line.substr(second + 1);; path.resize(path.rfind('/'))) {
                    if (v2) Tighten(cgroupRoot + path, true);
                    else for (const char* v1 : { "/cpu", "/cpu,cpuacct" }) Tighten(cgroupRoot + v1 + path, false);
                    if (path.empty()) break;
                }
            }

            #elif defined(_WIN32)

            alignas(4) USHORT group  = 0;
                       USHORT groups = 1;

            DWORD_PTR process = 0, system = 0;

            if (GetProcessGroupAffinity(GetCurrentProcess(), &groups, &group))
                count = GetProcessAffinityMask(GetCurrentProcess(), &process, &system) ?
                        static_cast<size_t>(std::popcount(static_cast<uint64_t>(process))) : 0;

            else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

            JOBOBJECT_CPU_RATE_CONTROL_INFORMATION rate {};

            const auto cpuRateInfo =
                QueryInformationJobObject(nullptr, JobObjectCpuRateControlInformation, &rate, sizeof(rate), nullptr);


            if (cpuRateInfo && (rate.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_ENABLE)) {

                const DWORD cap = rate.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP     ? rate.CpuRate
                                : rate.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE ? rate.MaxRate : 0;

                if (cap > 0 && cap < 10000)
                    quota = static_cast<size_t>(cap) * GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) / 10000;
            }

            #elif defined(__APPLE__)

            int    active = 0;
            size_t size   = sizeof(active);

            if (sysctlbyname("hw.activecpu", &active, &size, nullptr, 0) == 0 && active > 0)
                count = static_cast<size_t>(active);

            #endif

            if (count == 0) count = std::thread::hardware_concurrency();

            return std::max<size_t>(std::min(count, std::max<size_t>(quota, 1)), 1);
        }();

        return limit;
    }

    inline ThreadPool ThreadPool (1);

} // StockDory

#endif //STOCKDORY_THREADPOOL_H
