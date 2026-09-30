//
// Copyright (c) 2025-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_COMMON_H
#define STOCKDORY_COMMON_H

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <mutex>

#include <windows.h>

#endif

#ifdef __linux__

#include <sys/mman.h>
#include <unistd.h>

#endif

#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>

#include "../Backend/Misc.h"
#include "../Backend/Type/Move.h"

namespace StockDory
{

    [[noreturn]]
    void UnrecoverableError()
    {
        std::cerr << "An unrecoverable error has occurred." << std::endl;
        std::abort();
    }

    using Score = int32_t;

    constexpr Score Mate = 31000;
    constexpr Score Draw =     0;

    constexpr Score Infinity = Mate + 1;
    constexpr Score     None = Mate + 2;

    constexpr uint8_t MaxDepth = 246;
    constexpr uint8_t MaxMove  = 218;

    constexpr Score MateInMaxDepth = Mate - MaxDepth * 4;

    constexpr uint16_t LMRQuantization             =  1024;
    constexpr uint16_t MaterialScalingQuantization = 16384;

    constexpr uint16_t HistoryLimit = 16384;

    constexpr size_t  CorrectionHistorySize         = 16384;
    constexpr int32_t CorrectionHistoryLimit        =  1024;
    constexpr int32_t CorrectionHistoryQuantization =  1024;

    constexpr size_t MB = 1024 * 1024;

    constexpr size_t CacheLineSize = 64;

    using MS = std::chrono::milliseconds;
    using TP = std::chrono::time_point<std::chrono::steady_clock>;

    using KTable = Array<Move, 2, MaxDepth>;
    using HTable = Array<int16_t, 2, 6, 64>;

    using  CHTable = Array<HTable, 6, 64>;
    using CCHTable = Array<int16_t, 2, 6, 64, 6, 64>;

    using MinorCTable = Array<int16_t, 2, CorrectionHistorySize>;
    using MajorCTable = Array<MinorCTable, 2>;

    constexpr HTable NullHistory = {};

    bool IsMate(const Score score) { return abs(score) >= MateInMaxDepth; }

    bool IsWin (const Score score) { return score >=  MateInMaxDepth; }
    bool IsLoss(const Score score) { return score <= -MateInMaxDepth; }

    Score  WinIn(const uint8_t ply) { return  Mate - ply; }
    Score LossIn(const uint8_t ply) { return -Mate + ply; }

    Score PlyToMate(const Score score)
    {
        return IsWin (score) ?  Mate - score :
               IsLoss(score) ? -Mate - score : 0;
    }

    namespace Memory
    {

        // Uncomment to test cross-platform fallback implementation
        // #undef _WIN32
        // #undef __linux__

        #ifdef _WIN32

        void* AllocateHugePages(const size_t bytes)
        {
            const size_t pageSize = GetLargePageMinimum();

            if (!pageSize || bytes < pageSize || bytes > std::numeric_limits<size_t>::max() - (pageSize - 1))
                return nullptr;

            const size_t size = ((bytes + pageSize - 1) / pageSize) * pageSize;

            static std::mutex mutex;
            const std::lock_guard lock (mutex);

            HANDLE token = nullptr;
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return nullptr;

            TOKEN_PRIVILEGES requested {};

            requested.PrivilegeCount = 1;
            requested.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (!LookupPrivilegeValueA(nullptr, "SeLockMemoryPrivilege", &requested.Privileges[0].Luid)) {
                CloseHandle(token);
                return nullptr;
            }

            TOKEN_PRIVILEGES previous {};

            DWORD previousSize = sizeof(previous);

            const BOOL adjusted = AdjustTokenPrivileges(
                token, FALSE, &requested, previousSize, &previous,
                &previousSize
            );

            void* memory = nullptr;

            const DWORD error = GetLastError();

            if (adjusted && error == ERROR_SUCCESS)
                memory = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);

            if (adjusted && previous.PrivilegeCount)
                AdjustTokenPrivileges(token, FALSE, &previous, 0, nullptr, nullptr);

            CloseHandle(token);

            return memory;
        }

        void* Allocate(const size_t bytes, const size_t)
        {
            void* memory = AllocateHugePages(bytes);

            if (!memory) memory = VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (!memory) UnrecoverableError();

            return memory;
        }

        void Deallocate(void* memory, const size_t, const size_t) noexcept { VirtualFree(memory, 0, MEM_RELEASE); }

        #endif

        #ifdef __linux__

        size_t NormalPageSize()
        {
            static const size_t size = [] -> size_t
            {
                const long value = sysconf(_SC_PAGESIZE);

                if (value <= 0) UnrecoverableError();

                return static_cast<size_t>(value);
            }();

            return size;
        }

        size_t HugePageSize()
        {
            static const size_t size = [] -> size_t
            {
                size_t value = 0;

                if (FILE* file = std::fopen("/sys/kernel/mm/transparent_hugepage/hpage_pmd_size", "r")) {
                    if (std::fscanf(file, "%zu", &value) != 1) value = 0;

                    std::fclose(file);
                }

                const size_t page = NormalPageSize();

                return value >= page && std::has_single_bit(value) ? value : page;
            }();

            return size;
        }

        void* Allocate(const size_t bytes, const size_t)
        {
            const size_t page  = NormalPageSize();

            if (bytes > std::numeric_limits<size_t>::max() - (page - 1)) UnrecoverableError();

            const size_t size      = (bytes + page - 1) / page * page;
            const size_t alignment = HugePageSize();

            auto memory = MAP_FAILED;

            if (size <= std::numeric_limits<size_t>::max() - alignment) {
                void* mapping = mmap(
                    nullptr, size + alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
                );

                if (mapping != MAP_FAILED) {
                    const size_t offset = (alignment - reinterpret_cast<uintptr_t>(mapping) % alignment) % alignment;

                    auto* aligned = static_cast<std::byte*>(mapping) + offset;

                    if (offset && munmap(mapping       ,             offset) != 0) UnrecoverableError();
                    if (          munmap(aligned + size, alignment - offset) != 0) UnrecoverableError();

                    memory = aligned;
                }
            }

            if (memory == MAP_FAILED)
                memory = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            if (memory == MAP_FAILED) UnrecoverableError();

            madvise(memory, size, MADV_HUGEPAGE);
            return memory;
        }

        void Deallocate(void* memory, const size_t bytes, const size_t) noexcept { munmap(memory, bytes); }

        #endif

        #if !defined(_WIN32) && !defined(__linux__)

        void* Allocate(const size_t bytes, const size_t alignment)
        {
            if (alignment > alignof(std::max_align_t))
                return ::operator new(bytes, static_cast<std::align_val_t>(alignment));

            return ::operator new(bytes);
        }

        void Deallocate(void* memory, const size_t, const size_t alignment) noexcept
        {
            if (alignment > alignof(std::max_align_t))
                 ::operator delete(memory, static_cast<std::align_val_t>(alignment));
            else ::operator delete(memory                                          );
        }

        #endif

        template<typename T>
        class HugePageAllocator
        {

            static_assert(alignof(T) <= 4096);

            public:
            using value_type = T;

            using is_always_equal = std::true_type;

            constexpr HugePageAllocator() noexcept = default;

            template<typename U>
            constexpr HugePageAllocator(const HugePageAllocator<U>&) noexcept {}

            [[nodiscard]]
            // ReSharper disable once CppMemberFunctionMayBeStatic
            T* allocate(const size_t count)
            {
                if (!count) return nullptr;

                if (count > std::numeric_limits<size_t>::max() / sizeof(T)) UnrecoverableError();

                return static_cast<T*>(Allocate(count * sizeof(T), alignof(T)));
            }

            // ReSharper disable once CppMemberFunctionMayBeStatic
            void deallocate(T* memory, const size_t count) noexcept
            { if (memory) Deallocate(memory, count * sizeof(T), alignof(T)); }

            template<typename U>
            constexpr bool operator ==(const HugePageAllocator<U>&) const noexcept { return true; }

        };

    }

} // StockDory

#endif //STOCKDORY_COMMON_H
