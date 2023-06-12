//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#ifndef STOCKDORY_EVALUATION_H
#define STOCKDORY_EVALUATION_H

#include <MantaRay/Perspective/PerspectiveNNUE.h>
#include <MantaRay/Activation/ClippedReLU.h>

#include "../Backend/Type/PieceColor.h"
#include "../Backend/Type/Square.h"

#include "NetworkArchitecture.h"
#include "Model/NeuralNetworkBinary.h"

namespace StockDory
{

    class Evaluation
    {

        private:
            static Starshard NN;

        public:
            static inline void ResetNetworkState()
            {
                NN.  ResetAccumulator();
                NN.RefreshAccumulator();
            }

            static inline void PreMove()
            {
                NN.PushAccumulator();
            }

            static inline void PreUndoMove()
            {
                NN.PullAccumulator();
            }

            static inline void Activate  (const Piece piece, const Color color, const Square sq)
            {
                NN.EfficientlyUpdateAccumulator<MantaRay::AccumulatorOperation::Activate  >(piece, color, sq);
            }

            static inline void Deactivate(const Piece piece, const Color color, const Square sq)
            {
                NN.EfficientlyUpdateAccumulator<MantaRay::AccumulatorOperation::Deactivate>(piece, color, sq);
            }

            static inline void Transition(const Piece piece, const Color color, const Square from, const Square to)
            {
                NN.EfficientlyUpdateAccumulator(piece, color, from, to);
            }

            static inline int32_t Evaluate(Color color)
            {
                return NN.Evaluate(color);
            }

            template<Color Color>
            static inline int32_t Evaluate()
            {
                return NN.Evaluate(Color);
            }

    };

} // StockDory

Starshard StockDory::Evaluation::NN = []() {
    MantaRay::BinaryMemoryStream stream(_NeuralNetworkBinaryData, _NeuralNetworkBinarySize);
    return Starshard(stream);
}();

#endif //STOCKDORY_EVALUATION_H