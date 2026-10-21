//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_EVALUATION_H
#define STOCKDORY_EVALUATION_H

#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Color.h"

#include "Common.h"
#include "NetworkArchitecture.h"
#include "Model/NeuralNetworkBinary.h"

namespace StockDory
{

    class Evaluation
    {

        using Network = Flare     ;
        using   Stack = FlareStack;

        static inline Network NN;

        [[maybe_unused]]
        static inline const bool NetworkLoaded = [] -> bool
        {
            const MantaRay::LoadResult result = NN.Load(std::as_bytes(std::span(_NeuralNetworkBinaryData)));

            if (!result) {
                std::cerr << "ERROR: Failed to load the embedded neural network: " << result.Message()
                          << " (tensor " << result.Tensor << ", index " << result.Index << ")" << std::endl;
                std::abort();
            }

            return true;
        }();

        static inline std::vector<Stack> ThreadLocalStack;

        public:
        static std::string Name()
        {
            return "Flare";
        }

        static void Initialize()
        {
            const size_t threadCount = ThreadPool.Size();

            ThreadLocalStack.clear();
            ThreadLocalStack.reserve(threadCount);

            for (size_t i = 0; i < threadCount; i++) ThreadLocalStack.emplace_back(NN);
        }

        template<typename PieceBoards>
        static void Reset(const PieceBoards& boards, const size_t threadId = 0)
        {
            Stack& stack = ThreadLocalStack[threadId];

            stack.Reset(boards);
        }

        [[clang::always_inline]]
        static void Push(const MantaRay::Delta& delta, const size_t threadId = 0)
        {
            Stack& stack = ThreadLocalStack[threadId];

            stack.Push(delta);
        }

        [[clang::always_inline]]
        static void Pop(const size_t threadId = 0)
        {
            Stack& stack = ThreadLocalStack[threadId];

            stack.Pop();
        }

        [[clang::always_inline]]
        static Score Evaluate(const Color color, const size_t threadId = 0)
        {
            Stack& stack = ThreadLocalStack[threadId];

            return stack.Evaluate(color);
        }

    };

} // StockDory

#endif //STOCKDORY_EVALUATION_H
