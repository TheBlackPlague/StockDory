//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_EVALUATION_H
#define STOCKDORY_EVALUATION_H

#include <vector>

#include "../Backend/ThreadPool.h"
#include "../Backend/Type/Square.h"

#include "Common.h"
#include "NetworkArchitecture.h"
#include "Model/NeuralNetworkBinary.h"

namespace StockDory
{

    class Evaluation
    {

        static inline Aurora NN = [] -> Aurora
        {
            MantaRay::BinaryMemoryStream stream (_NeuralNetworkBinaryData, sizeof _NeuralNetworkBinaryData);
            return Aurora(stream);
        }();

        static inline std::vector<AuroraStack> ThreadLocalStack;

        public:
        static std::string Name()
        {
            return "Aurora";
        }

        static void Initialize()
        {
            const size_t threadCount = ThreadPool.Size() + 1;

            ThreadLocalStack.clear();
            ThreadLocalStack.reserve(threadCount);

            for (size_t i = 0; i < threadCount; i++) {
                ThreadLocalStack.emplace_back();
                NN.Refresh(*ThreadLocalStack[i]);
            }
        }

        static void ResetNetworkState(const size_t threadId = 0)
        {
            AuroraStack& stack = ThreadLocalStack[threadId];

            stack.Reset();
            NN.Refresh(*stack);
        }

        [[clang::always_inline]]
        static void PreMove(const size_t threadId = 0)
        {
            AuroraStack& stack = ThreadLocalStack[threadId];
            stack++;
        }

        [[clang::always_inline]]
        static void PreUndoMove(const size_t threadId = 0)
        {
            AuroraStack& stack = ThreadLocalStack[threadId];
            stack--;
        }

        [[clang::always_inline]]
        static void Activate(const Piece piece, const Color color, const Square sq, const size_t threadId = 0)
        {
            AuroraStack& stack = ThreadLocalStack[threadId];

            NN.Insert(piece, color, sq, *stack);
        }

        [[clang::always_inline]]
        static void Deactivate(const Piece piece, const Color color, const Square sq, const size_t threadId = 0)
        {
            AuroraStack& stack = ThreadLocalStack[threadId];

            NN.Remove(piece, color, sq, *stack);
        }

        [[clang::always_inline]]
        static void Transition(const Piece piece, const Color color, const Square from, const Square to,
                               const size_t threadId = 0)
        {
            AuroraStack& stack = ThreadLocalStack[threadId];

            NN.Move(piece, color, from, to, *stack);
        }

        [[clang::always_inline]]
        static Score Evaluate(const Color color, const size_t threadId = 0)
        {
            AuroraStack& stack = ThreadLocalStack[threadId];

            return NN.Evaluate(color, *stack);
        }

    };

} // StockDory

#endif //STOCKDORY_EVALUATION_H
