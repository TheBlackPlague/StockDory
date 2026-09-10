//
// Copyright (c) 2023-2026 Shaheryar Sohail and Lee Durbin
// SPDX-License-Identifier: AGPL-3.0-only
//

#include <algorithm>
#include <functional>
#include <iostream>
#include <ranges>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Backend/Board.h"
#include "Engine/OrderedMoveList.h"

#include "PerftPositions.h"

using namespace StockDory;

constexpr MoveType EngineMove = STANDARD | ZOBRIST | NNUE;

size_t PositionsChecked = 0;
size_t MovesChecked = 0;
Array<size_t, 16> FlagsChecked {};

void Require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

template<BoardType Profile>
void Equivalent(const Board& expected, const BasicBoard<Profile>& actual)
{
    const std::string fen = expected.Fen();
    Require(actual.Fen() == fen, "FEN differs: " + fen);
    Require(actual.ColorToMove() == expected.ColorToMove(), "Side to move differs: " + fen);
    Require(actual.CastlingRights() == expected.CastlingRights(), "Castling rights differ: " + fen);
    Require(actual.EnPassantSquare() == expected.EnPassantSquare(), "En passant differs: " + fen);

    for (Color color = White; color != NAC; color = Next(color)) {
        Require(actual[color] == expected[color], "Color occupancy differs: " + fen);
        for (Piece piece = Pawn; piece != NAP; piece = Next(piece))
            Require(actual.PieceBoard(piece, color) == expected.PieceBoard(piece, color),
                    "Piece bitboard differs: " + fen);
    }
    Require(actual[NAC] == expected[NAC], "Empty occupancy differs: " + fen);
    for (Square square = A1; square != NASQ; square = Next(square))
        Require(actual[square].Piece() == expected[square].Piece() && actual[square].Color() == expected[square].Color(),
                "Mailbox differs: " + fen);

    if constexpr (Profile != BoardType::Packed)
        Require(actual.Zobrist() == expected.Zobrist(), "Zobrist differs: " + fen);
}

void VerifyEvaluation(const Board& board)
{
    board.LoadForEvaluation(1);
    Require(Evaluation::Evaluate(White, 0) == Evaluation::Evaluate(White, 1),
            "White incremental NNUE differs from refresh: " + board.Fen());
    Require(Evaluation::Evaluate(Black, 0) == Evaluation::Evaluate(Black, 1),
            "Black incremental NNUE differs from refresh: " + board.Fen());
}

void VerifyPosition(const Board& board, const PerftBoard& perft)
{
    const Board rebuilt (board.Fen());
    const PerftBoard converted (board);
    const PackedBoard packed (board);
    const Board fromPerft (perft);
    const Board fromPacked (packed);
    const PerftBoard unpacked (packed);
    const PackedBoard packedPerft (perft);

    Equivalent(board, rebuilt);
    Equivalent(board, perft);
    Equivalent(board, converted);
    Equivalent(board, packed);
    Equivalent(board, fromPerft);
    Equivalent(board, fromPacked);
    Equivalent(board, unpacked);
    Require(packed.Data() == packedPerft.Data(), "Packed representation differs by source profile");
    Require(PackedBoard(fromPacked).Data() == packed.Data(), "Packed bytes changed after round trip");
    Require((board[White] & board[Black]) == BBDefault, "Overlapping color occupancy");
    Require((board[White] | board[Black] | board[NAC]) == BBFilled, "Incomplete occupancy");

    VerifyEvaluation(board);
    PositionsChecked++;
}

template<Color Color>
std::vector<::Move> EngineMoves(const Board& board)
{
    const KTable killers {};
    const HTable history {};
    const OrderedMoveList<Color> generated (board, 0, killers, history);
    std::vector<::Move> moves;
    for (uint8_t i = 0; i < generated.Count(); i++) moves.push_back(generated.UnsortedAccess(i));
    return moves;
}

template<Piece Piece, Color Color>
void AppendPerftMoves(const PerftBoard& board, const PinBitBoard& pin, const CheckBitBoard& check,
                      std::vector<::Move>& moves)
{
    BitBoardIterator pieces (board.PieceBoard<Color>(Piece));
    for (Square from = pieces.Value(); from != NASQ; from = pieces.Value()) {
        const MoveList<Piece, Color, BoardType::Perft> generated (board, from, pin, check);
        BitBoardIterator destinations = generated.Iterator();
        for (Square to = destinations.Value(); to != NASQ; to = destinations.Value()) {
            if (generated.Promotion(from))
                for (const auto promotion : {Knight, Bishop, Rook, Queen})
                    moves.push_back(board.CreateMove<Piece>(from, to, promotion));
            else moves.push_back(board.CreateMove<Piece>(from, to));
        }
    }
}

template<Color Color>
std::vector<::Move> PerftMoves(const PerftBoard& board)
{
    const PinBitBoard pin = board.Pin<Color, Opposite(Color)>();
    const CheckBitBoard check = board.Check<Opposite(Color)>();
    std::vector<::Move> moves;
    if (!check.DoubleCheck) {
        AppendPerftMoves<Pawn,   Color>(board, pin, check, moves);
        AppendPerftMoves<Knight, Color>(board, pin, check, moves);
        AppendPerftMoves<Bishop, Color>(board, pin, check, moves);
        AppendPerftMoves<Rook,   Color>(board, pin, check, moves);
        AppendPerftMoves<Queen,  Color>(board, pin, check, moves);
    }
    AppendPerftMoves<King, Color>(board, pin, check, moves);
    return moves;
}

void VerifyFlags(const Board& board, const ::Move move)
{
    const Piece piece = board[move.From()].Piece();
    const bool pawn = piece == Pawn;
    const bool diagonal = move.From() % 8 != move.To() % 8;
    const bool occupied = board[move.To()].Piece() != NAP;
    const bool enPassant = pawn && diagonal && !occupied;
    const bool promotion = pawn && (move.To() < A2 || move.To() >= A8);
    const bool castling = piece == King && std::abs(static_cast<int>(move.To()) - move.From()) == 2;
    const bool doublePush = pawn && std::abs(static_cast<int>(move.To()) - move.From()) == 16;
    const std::string context = board.Fen() + " " + move.ToString();

    Require(move.Capture() == (occupied || enPassant), "Capture flag differs from board: " + context);
    Require(move.EnPassant() == enPassant, "En passant flag differs from board: " + context);
    Require(move.Castling() == castling, "Castling flag differs from board: " + context);
    Require(move.DoublePush() == doublePush, "Double-push flag differs from board: " + context);
    Require((move.Promotion() != NAP) == promotion, "Promotion flag differs from board: " + context);
    if (promotion) Require(move.Promotion() >= Knight && move.Promotion() <= Queen, "Invalid promotion piece");
    Require(move.SameIdentity(::Move::FromString(move.ToString())), "Generated move lost UCI identity");
    FlagsChecked[static_cast<uint8_t>(move.Flags())]++;
    MovesChecked++;
}

std::vector<::Move> LegalMoves(const Board& board, const PerftBoard& perft)
{
    auto moves = board.ColorToMove() == White ? EngineMoves<White>(board) : EngineMoves<Black>(board);
    auto comparison = perft.ColorToMove() == White ? PerftMoves<White>(perft) : PerftMoves<Black>(perft);
    const auto order = [](const ::Move first, const ::Move second) { return first.ToString() < second.ToString(); };
    std::ranges::sort(moves, order);
    std::ranges::sort(comparison, order);
    Require(moves == comparison, "Engine and PERFT profiles generate different moves: " + board.Fen());
    for (const auto move : moves) VerifyFlags(board, move);
    return moves;
}

void VerifyNull(Board& board, PerftBoard& perft)
{
    const Board before = board;
    const auto engineState = board.Move();
    const auto perftState = perft.Move();
    Require(board.ColorToMove() == Opposite(before.ColorToMove()) && board.EnPassantSquare() == NASQ,
            "Null move failed to change side or clear en passant");
    VerifyPosition(board, perft);
    board.UndoMove(engineState);
    perft.UndoMove(perftState);
    Equivalent(before, board);
    VerifyPosition(board, perft);
}

void VerifyLegalAfterMove(const Board& board)
{
    const bool illegal = board.ColorToMove() == White ? board.Checked<Black>() : board.Checked<White>();
    Require(!illegal, "Generated move left its own king in check: " + board.Fen());
}

void Playout(const std::string_view fen, uint64_t seed)
{
    Board board (fen);
    PerftBoard perft (board);
    board.LoadForEvaluation(0);
    VerifyPosition(board, perft);
    VerifyNull(board, perft);

    const auto initial = LegalMoves(board, perft);
    for (const auto move : initial) {
        const Board before = board;
        const auto engineState = board.Move<EngineMove>(move, 0);
        const auto perftState = perft.Move<STANDARD>(move);
        VerifyLegalAfterMove(board);
        VerifyPosition(board, perft);
        board.UndoMove<EngineMove>(engineState, move, 0);
        perft.UndoMove<STANDARD>(perftState, move);
        Equivalent(before, board);
        VerifyPosition(board, perft);
    }

    struct Step
    {

        Board Before;
        ::Move Move;
        PreviousState EngineState;
        PreviousState PerftState;

    };

    std::vector<Step> path;
    for (size_t ply = 0; ply < 96; ply++) {
        const auto moves = LegalMoves(board, perft);
        if (moves.empty()) break;
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        const auto move = moves[seed % moves.size()];
        const Board before = board;
        const auto engineState = board.Move<EngineMove>(move, 0);
        const auto perftState = perft.Move<STANDARD>(move);
        path.push_back({before, move, engineState, perftState});
        VerifyLegalAfterMove(board);
        VerifyPosition(board, perft);
        if (ply % 11 == 0) VerifyNull(board, perft);
    }
    for (const auto& step : std::ranges::reverse_view(path)) {
        board.UndoMove<EngineMove>(step.EngineState, step.Move, 0);
        perft.UndoMove<STANDARD>(step.PerftState, step.Move);
        Equivalent(step.Before, board);
        VerifyPosition(board, perft);
    }
}

int main()
{
    static_assert(sizeof(PackedBoard) == 24);
    static_assert(std::is_trivially_copyable_v<Board>);
    static_assert(std::is_trivially_copyable_v<PerftBoard>);
    static_assert(std::is_trivially_copyable_v<PackedBoard>);

    StockDory::ThreadPool.Resize(2);
    Evaluation::Initialize();
    uint64_t seed = 0x53444F5259504552ULL;
    for (const auto& position : Testing::PerftPositions) Playout(position.Fen, ++seed);

    for (const auto fen : {
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
        "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
        "4k3/P6P/8/8/8/8/p6p/4K3 w - - 0 1",
        "4k3/P6P/8/8/8/8/p6p/4K3 b - - 0 1",
        "r3k2r/1P4P1/8/8/8/8/1p4p1/R3K2R w KQkq - 0 1",
        "r3k2r/1P4P1/8/8/8/8/1p4p1/R3K2R b KQkq - 0 1",
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1",
        "4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 1",
        "4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1"
    }) Playout(fen, ++seed);

    for (const uint8_t flag : {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15})
        Require(FlagsChecked[flag] > 0, "A legal move flag was not exercised");

    std::cout << PositionsChecked << " positions, " << MovesChecked
              << " generated moves: conversions, flags, make/undo/null, hashes and NNUE passed\n";
}
