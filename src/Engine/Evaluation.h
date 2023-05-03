//
// Copyright (c) 2023 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#ifndef STOCKDORY_EVALUATION_H
#define STOCKDORY_EVALUATION_H

#include <MantaRay/Perspective/PerspectiveNNUE.h>
#include <MantaRay/Activation/ClippedReLU.h>

#include "../Backend/Type/PieceColor.h"
#include "../Backend/Type/Square.h"

#include "../Backend/Board.h"

#include "Model/NeuralNetworkBinary.h"

namespace StockDory
{

    class Evaluation
    {

        using ActivationFunction = MantaRay::ClippedReLU<int16_t, 0, 255>;
        using NNUE = MantaRay::PerspectiveNetwork<int16_t, int32_t, ActivationFunction, 768, 256, 1, 512, 400, 255, 64>;

        private:
            static NNUE NeuralNetwork;

        public:
            static inline void ResetNetworkState(const Board& board)
            {
                NeuralNetwork.  ResetAccumulator();
                NeuralNetwork.RefreshAccumulator();

                for (Square sq = A1; sq < NASQ; sq = Next(sq)) {
                    PieceColor pc = board[sq];
                    if (pc.Piece() == NAP || pc.Color() == NAC) continue;

                    NeuralNetwork.EfficientlyUpdateAccumulator<MantaRay::AccumulatorOperation::Activate>(
                            pc.Piece(),
                            pc.Color(),
                            sq
                    );
                }
            }

            static inline int32_t Evaluate(const Board& board)
            {
                return NeuralNetwork.Evaluate(board.ColorToMove());
            }

    };

} // StockDory

StockDory::Evaluation::NNUE StockDory::Evaluation::NeuralNetwork = []() {
    MantaRay::BinaryMemoryStream stream(_NeuralNetworkBinaryData, _NeuralNetworkBinarySize);
    return NNUE(stream);
}();

#endif //STOCKDORY_EVALUATION_H

#pragma clang diagnostic pop