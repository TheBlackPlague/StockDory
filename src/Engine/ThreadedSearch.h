//
// Copyright (c) 2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_THREADEDSEARCH_H
#define STOCKDORY_THREADEDSEARCH_H

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "Search.h"
#include "ThreadPool.h"

namespace StockDory
{

    struct ThreadedSearch
    {

        static inline SearchTask<Main> MainTask;

        static inline Limit           Limit      {};
        static inline Board           Board      {};
        static inline RepetitionStack Repetition {};

        static inline uint8_t HalfMoveClock = 0;

        static inline Atomic<uint32_t> ParallelPreparing { 0 };
        static inline Atomic<uint32_t> ParallelSearching { 0 };

        static inline SearchResult Result {};

        static void Resize(const size_t threads)
        {
            Stop();
            Wait();

            Release();

            ThreadPool.Resize(threads);

            ParallelTaskPool.assign(ThreadPool.Size() - 1, nullptr);

            ThreadPool.Run(
                [](const size_t index) -> void
                {
                    if (index != 0) ParallelTaskPool[index - 1] = new SearchTask<Parallel>();
                    else ReplaceInline(MainTask);
                }
            );
        }

        static void Release()
        {
            for (const SearchTask<Parallel>* task : ParallelTaskPool) delete task;

            ParallelTaskPool.clear();
        }

        static void Reset()
        {
            Stop();
            Wait();

            ThreadPool.Run(
                [](const size_t index) -> void
                {
                    if      (index == 0                      ) ReplaceInline(MainTask);
                    else if (index <= ParallelTaskPool.size()) ReplaceInline(*ParallelTaskPool[index - 1]);
                }
            );
        }

        static void Start(
            const StockDory::Limit&          limit,
            const StockDory::Board&          board,
            const RepetitionStack&      repetition,
            const uint8_t            halfMoveClock)
        {
            Stop();
            Wait();

            Limit         = limit        ;
            Board         = board        ;
            Repetition    = repetition   ;
            HalfMoveClock = halfMoveClock;

            const uint32_t parallel = static_cast<uint32_t>(ParallelTaskPool.size());

            SearchStopped    .Store(false   , MemoryOrder::relaxed);
            ParallelPreparing.Store(parallel, MemoryOrder::relaxed);
            ParallelSearching.Store(parallel, MemoryOrder::relaxed);

            ThreadPool.Launch([](const size_t index) -> void { Run(index); });
        }

        static void Stop()
        {
            SearchStopped.Store(true, MemoryOrder::relaxed);
        }

        static void Wait() { ThreadPool.Wait(); }

        static bool Searching() { return ThreadPool.Busy(); }

        static SearchResult Best(const std::chrono::nanoseconds time)
        {
            return MainTask.Summary(MainTask.TotalNodes(), time);
        }

        template<SearchThreadType ThreadType>
        [[clang::noinline]] // the "noinline" here is important so we discard the stack usage
        static void Prepare(SearchTask<ThreadType>& task)
        {
            auto persistent = static_cast<const SearchPersistentData&>(task);

            persistent.Decay();

            ReplaceInline(task, Limit, Board, Repetition, HalfMoveClock, persistent);
        }

        static void Run(const size_t index)
        {
            // Symmetric MultiProcessing (SMP):
            //
            // Relevant links:
            // - http://en.wikipedia.org/wiki/Symmetric_multiprocessing
            // - https://www.chessprogramming.org/Lazy_SMP
            //
            // Assuming there is more than one thread available for the search, a SMP approach is used to get a much
            // more accurate search. This approach relies on constructive and destructive interference of multiple
            // threads through the transposition table shared between them - the main thread is running the main search
            // task, while the other threads are running parallel search tasks that are constructively and destructively
            // interfering with the main search task (and with each other).
            //
            // Destructive interference happens when one task overwrites the transposition table entry another task is
            // about to read - this prevents the task from reading the entry and forces it to search the position again,
            // potentially disabling certain pruning techniques and enabling others. Regardless, the search space
            // explored by the task gets widened, and the search is more accurate, however, searching the extra branches
            // causes the search to take longer to complete; a consequence of destructive interference.
            //
            // Constructive interference happens when one task provides a transposition table entry that another task
            // is about to read - this allows the task to read the entry and use it to speed through certain branches by
            // returning directly from the transposition table if possible. Where this isn't possible, the task can use
            // the entry's evaluation as the static evaluation for that branch, enabling certain pruning techniques and
            // disabling others, once again widening the search space explored by the task. Furthermore, the provided
            // entry's move can be used to speed through the branch since the move policy ensures that the move is
            // the first move in the move list. This can allow the search for that branch to complete much faster,
            // overall reducing the search time, mitigating the destructive interference's negative impact on the
            // search time.
            //
            // Together, the search space is considerably widened, making the search more accurate, while the search
            // time remains relatively the same on average. This leads the engine to find better moves in the same
            // amount of time and avoid some pitfalls of the heuristical pruning, reduction, and search techniques used
            if (index != 0) {
                SearchTask<Parallel>& task = *ParallelTaskPool[index - 1];

                Prepare(task);

                if (ParallelPreparing.Subtract(1, MemoryOrder::release) == 0) ParallelPreparing.NotifyAll();

                task.IterativeDeepening();

                if (ParallelSearching.Subtract(1, MemoryOrder::release) == 0) ParallelSearching.NotifyAll();

                return;
            }

            Prepare(MainTask);

            while (const uint32_t building = ParallelPreparing.Load(MemoryOrder::acquire))
                ParallelPreparing.Wait(building, MemoryOrder::acquire);

            const TP start = std::chrono::steady_clock::now();

            MainTask.IterativeDeepening();

            const auto time = std::chrono::steady_clock::now() - start;

            // The main thread is responsible for ensuring that it stops all the parallel tasks when it concludes the
            // search
            Stop();

            while (const uint32_t searching = ParallelSearching.Load(MemoryOrder::acquire))
                ParallelSearching.Wait(searching, MemoryOrder::acquire);

            Result = Best(time);

            // The main thread is responsible for notifying the handlers about the completion of the search, providing
            // them with the best move found
            EventHandler.HandleSearchCompletion({
                .Move = Result.Move,

                .Result = Result,

                .Board = Board
            });
        }

    };

} // StockDory

#endif //STOCKDORY_THREADEDSEARCH_H
