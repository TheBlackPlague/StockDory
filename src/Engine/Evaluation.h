//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#ifndef STOCKDORY_EVALUATION_H
#define STOCKDORY_EVALUATION_H

#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>

#include <MantaRay/MantaRay.h>

#include "Model/NeuralNetworkBinary.h"

#include "Board.h"

namespace StockDory
{

    // Architecture:
    namespace Networks
    {

        template<size_t Hidden, bool SquaredActivation = true>
        constexpr auto PerspectiveBase768()
        {
            using namespace MantaRay;

            auto [stm, ntm] = Input<Chess768>();

            const auto l0 = Affine(768, Hidden, Weights<i16>(255, Clip(1.98)), Biases<i16>(255, Clip(1.98)));
            const auto l1 = SquaredActivation ?
                Affine(2 * Hidden, 1, Weights<i16>(64, Clip(1.98)), Biases<i16>(255 * 64), DivideBy(255)) :
                Affine(2 * Hidden, 1, Weights<i16>(64, Clip(1.98)), Biases<i16>(255 * 64)               ) ;

            const auto yStm = SquaredActivation ? SquaredClippedReLU(l0(stm)) : ClippedReLU(l0(stm));
            const auto yNtm = SquaredActivation ? SquaredClippedReLU(l0(ntm)) : ClippedReLU(l0(ntm));

            const auto yConcat = Concat(yStm, yNtm);

            const auto y = l1(yConcat);

            return Output(y, 400);
        }

        constexpr auto Eclipse() { return PerspectiveBase768<1536>(); }

    } // Networks

    // Network:
    using Eclipse = MantaRay::Network<Networks::Eclipse>;

    constexpr std::string_view NetworkName = "Eclipse";

    // Accumulator Stack:
    constexpr size_t AccumulatorStackSize = MaxDepth + 1;

    // Defaults to lazy updates
    using NetworkStack = MantaRay::AccumulatorStack<Networks::Eclipse, AccumulatorStackSize>;

    using NetworkDelta = MantaRay::PieceDelta;

    inline Eclipse Network;

    [[maybe_unused]]
    inline const bool NetworkLoaded = [] -> bool
    {
        const MantaRay::LoadResult result = Network.Load(std::as_bytes(std::span(_NeuralNetworkBinaryData)));

        if (!result) {
            std::cerr << "ERROR: Failed to load the embedded neural network: " << result.Message()
                      << " (tensor " << result.Tensor << ", index " << result.Index << ")" << std::endl;

            std::abort();
        }

        return true;
    }();

    static_assert(
        Pawn   == std::to_underlying(MantaRay::Piece::Pawn  ) &&
        Knight == std::to_underlying(MantaRay::Piece::Knight) &&
        Bishop == std::to_underlying(MantaRay::Piece::Bishop) &&
        Rook   == std::to_underlying(MantaRay::Piece::Rook  ) &&
        Queen  == std::to_underlying(MantaRay::Piece::Queen ) &&
        King   == std::to_underlying(MantaRay::Piece::King  )
    );

    static_assert(White == MantaRay::White && Black == MantaRay::Black);

    template<typename T>
    struct NetworkBoardView
    {

        const T& Board;

        MantaRay::u64 Pieces(const MantaRay::u08 side, const MantaRay::Piece piece) const
        {
            return Board.PieceBoard(static_cast<PieceType>(piece), static_cast<Side>(side));
        }

        MantaRay::u08 SideToMove() const { return Board.SideToMove(); }

        MantaRay::u08 King(const MantaRay::u08 side) const
        {
            return ToSquare(Board.PieceBoard(PieceType::King, static_cast<Side>(side)));
        }

        MantaRay::u64 Occupied() const { return Board.Occupied(); }

        MantaRay::u08 At(const MantaRay::u08 sq) const
        {
            const Piece piece = Board[static_cast<Square>(sq)];

            return piece.Type() == NAPT ? 12 : piece.Side() * 6 + piece.Type();
        }

    };

    class AccumulatorDelta
    {

        NetworkDelta Internal {};

        friend class Evaluator;

        public:
        [[clang::always_inline]]
        void Remove    (const PieceType type, const Side side, const Square sq)
        {
            Internal.Remove(static_cast<MantaRay::Piece>(type), side, sq);
        }

        [[clang::always_inline]]
        void Insert    (const PieceType type, const Side side, const Square sq)
        {
            Internal.Insert(static_cast<MantaRay::Piece>(type), side, sq);
        }

        [[clang::always_inline]]
        void Transition(const PieceType type, const Side side, const Square from, const Square to)
        {
            Internal.Move(static_cast<MantaRay::Piece>(type), side, from, to);
        }

    };

    class Evaluator
    {

        mutable NetworkStack Internal { Network };

        public:
        Evaluator() {}

        // ReSharper disable once CppMemberFunctionMayBeConst
        void Reset(const Board& board) { Internal.Reset(NetworkBoardView { board }); }

        [[clang::always_inline]]
        // ReSharper disable once CppMemberFunctionMayBeConst
        void Push(const AccumulatorDelta& delta, const Board& after)
        {
            Internal.Push(delta.Internal, NetworkBoardView { after });
        }

        [[clang::always_inline]]
        // ReSharper disable once CppMemberFunctionMayBeConst
        void Pop() { Internal.Pop(); }

        [[clang::always_inline]]
        Score Evaluate(const Board& board) const { return Internal.Evaluate(NetworkBoardView { board }); }

    };

} // StockDory

#endif //STOCKDORY_EVALUATION_H
