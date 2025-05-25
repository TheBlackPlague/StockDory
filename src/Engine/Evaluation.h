//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_EVALUATION_H
#define STOCKDORY_EVALUATION_H

#include "../Backend/Type/Square.h"

#include "Common.h"
#include "NetworkArchitecture.h"
#include "Model/NeuralNetworkBinary.h"

namespace StockDory
{

    class Evaluation
    {

        static thread_local inline Aurora NN = [] -> Aurora
        {
            MantaRay::BinaryMemoryStream stream (_NeuralNetworkBinaryData, sizeof _NeuralNetworkBinaryData);
            return Aurora(stream);
        }();

        public:
        static std::string Name()
        {
            return "Aurora";
        }

        static void ResetNetworkState()
        {
            NN.Reset();
            NN.Refresh();
        }

        [[clang::always_inline]]
        static void PreMove()
        {
            NN.Push();
        }

        [[clang::always_inline]]
        static void PreUndoMove()
        {
            NN.Pop();
        }

        [[clang::always_inline]]
        static void Activate(const Piece piece, const Color color, const Square sq)
        {
            NN.Insert(piece, color, sq);
        }

        [[clang::always_inline]]
        static void Deactivate(const Piece piece, const Color color, const Square sq)
        {
            NN.Remove(piece, color, sq);
        }

        [[clang::always_inline]]
        static void Transition(const Piece piece, const Color color, const Square from, const Square to)
        {
            NN.Move(piece, color, from, to);
        }

        [[clang::always_inline]]
        static Score Evaluate(const Color color)
        {
            return NN.Evaluate(color);
        }

    };

} // StockDory

#endif //STOCKDORY_EVALUATION_H
