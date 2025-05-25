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

        static inline std::vector<Aurora> NNs;

        // static inline Aurora NN = [] -> Aurora
        // {
        //     MantaRay::BinaryMemoryStream stream (_NeuralNetworkBinaryData, sizeof _NeuralNetworkBinaryData);
        //     return Aurora(stream);
        // }();

        public:
        static std::string Name()
        {
            return "Aurora";
        }

        static void Initialize()
        {
            const size_t threadCount = ThreadPool.Size() + 1;

            NNs.clear();
            NNs.reserve(threadCount);

            for (size_t i = 0; i < threadCount; i++) {
                MantaRay::BinaryMemoryStream stream (_NeuralNetworkBinaryData, sizeof _NeuralNetworkBinaryData);
                NNs.emplace_back(stream);
            }
        }

        static void ResetNetworkState(const size_t threadId = 0)
        {
            NNs[threadId].Reset();
            NNs[threadId].Refresh();
        }

        [[clang::always_inline]]
        static void PreMove(const size_t threadId = 0)
        {
            NNs[threadId].Push();
        }

        [[clang::always_inline]]
        static void PreUndoMove(const size_t threadId = 0)
        {
            NNs[threadId].Pop();
        }

        [[clang::always_inline]]
        static void Activate(const Piece piece, const Color color, const Square sq, const size_t threadId = 0)
        {
            NNs[threadId].Insert(piece, color, sq);
        }

        [[clang::always_inline]]
        static void Deactivate(const Piece piece, const Color color, const Square sq, const size_t threadId = 0)
        {
            NNs[threadId].Remove(piece, color, sq);
        }

        [[clang::always_inline]]
        static void Transition(const Piece piece, const Color color, const Square from, const Square to,
                               const size_t threadId = 0)
        {
            NNs[threadId].Move(piece, color, from, to);
        }

        [[clang::always_inline]]
        static Score Evaluate(const Color color, const size_t threadId = 0)
        {
            return NNs[threadId].Evaluate(color);
        }

    };

} // StockDory

#endif //STOCKDORY_EVALUATION_H
