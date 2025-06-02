//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <pybind11/pybind11.h>

#include "Information.h"

#include "Backend/Board.h"
#include "Backend/Move/MoveList.h"
#include "Backend/Type/Color.h"
#include "Backend/Type/Move.h"
#include "Backend/Type/Piece.h"
#include "Backend/Type/Square.h"
#include "Engine/Search.h"

#include "External/strutil.h"

#include "Terminal/Perft/PerftRunner.h"
#include "Terminal/UCI/UCITime.h"

namespace py = pybind11;

constexpr size_t API_VERSION = 0;

class PyMoveList
{

    Array<Move, StockDory::MaxMove> Internal = {};

    uint8_t Size = 0;

    template<Color Color>
    void AddMoveLoop(const StockDory::Board& board, const Piece piece)
    {
        const   PinBitBoard pin   = board.Pin  <Color, Opposite(Color)>();
        const CheckBitBoard check = board.Check<       Opposite(Color)>();

        if (check.DoubleCheck && piece != King) return;

        switch (piece) {
            case Pawn  :
                AddMoveLoop<Color, Pawn  >(board, pin, check);
            break;
            case Knight:
                AddMoveLoop<Color, Knight>(board, pin, check);
            break;
            case Bishop:
                AddMoveLoop<Color, Bishop>(board, pin, check);
            break;
            case Rook  :
                AddMoveLoop<Color, Rook  >(board, pin, check);
            break;
            case Queen :
                AddMoveLoop<Color, Queen >(board, pin, check);
            break;
            case King  :
                AddMoveLoop<Color, King  >(board, pin, check);
            break;
            default:
                throw std::invalid_argument("Invalid piece.");
        }
    }

    template<Color Color>
    void AddMoveLoop(const StockDory::Board& board, const Piece piece, const Square sq)
    {
        const   PinBitBoard pin   = board.Pin  <Color, Opposite(Color)>();
        const CheckBitBoard check = board.Check<       Opposite(Color)>();

        if (check.DoubleCheck && piece != King) return;

        switch (piece) {
            case Pawn  :
                AddMoveLoop<Color, Pawn  >(board, sq, pin, check);
            break;
            case Knight:
                AddMoveLoop<Color, Knight>(board, sq, pin, check);
            break;
            case Bishop:
                AddMoveLoop<Color, Bishop>(board, sq, pin, check);
            break;
            case Rook  :
                AddMoveLoop<Color, Rook  >(board, sq, pin, check);
            break;
            case Queen :
                AddMoveLoop<Color, Queen >(board, sq, pin, check);
            break;
            case King  :
                AddMoveLoop<Color, King  >(board, sq, pin, check);
            break;
            default:
                throw std::invalid_argument("Invalid piece.");
        }
    }

    template<Color Color, Piece Piece>
    void AddMoveLoop(const StockDory::Board& board, const PinBitBoard& pin, const CheckBitBoard& check)
    {
        BitBoardIterator iterator (board.PieceBoard<Color>(Piece));

        for (Square sq = iterator.Value(); sq != NASQ; sq = iterator.Value())
            AddMoveLoop<Color, Piece>(board, sq, pin, check);
    }

    template<Color Color, Piece Piece>
    void AddMoveLoop(const StockDory::Board& board, const Square from,
                     const PinBitBoard& pin, const CheckBitBoard& check)
    {
        const StockDory::MoveList<Piece, Color> moves (board, from, pin, check);
        BitBoardIterator                        moveIterator = moves.Iterator();

        for (Square to = moveIterator.Value(); to != NASQ; to = moveIterator.Value()) {
            if (moves.Promotion(from)) {
                Internal[Size++] = Move(from, to, Queen );
                Internal[Size++] = Move(from, to, Knight);
                Internal[Size++] = Move(from, to, Rook  );
                Internal[Size++] = Move(from, to, Bishop);
            } else {
                Internal[Size++] = Move(from, to, NAP   );
            }
        }
    }

    public:
    PyMoveList(const StockDory::Board& board, const Piece piece, const Color color)
    {
        switch (color) {
            case White:
                AddMoveLoop<White>(board, piece);
                break;
            case Black:
                AddMoveLoop<Black>(board, piece);
                break;
            default:
                throw std::invalid_argument("Invalid color.");
        }
    }

    PyMoveList(const StockDory::Board& board, const Square sq)
    {
        switch (const PieceColor pc = board[sq]; pc.Color()) {
            case White:
                AddMoveLoop<White>(board, pc.Piece(), sq);
            break;
            case Black:
                AddMoveLoop<Black>(board, pc.Piece(), sq);
            break;
            default:
                throw std::invalid_argument("Invalid color.");
        }
    }

    Move operator [](const uint8_t idx) const
    {
        return Internal[idx];
    }

    uint8_t Count() const
    {
        return Size;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "[";

        for (size_t i = 0 ; i < Size; i++) {
            ss << Internal[i].ToString();

            if (i != Size - 1) ss << ", ";
        }

        ss << "]";

        return ss.str();
    }

    Array<Move, StockDory::MaxMove>::iterator Begin()
    {
        return Internal.begin();
    }

};

template<size_t T>
class PyConstantHolder {};

class PySearchHandler : StockDory::DefaultSearchEventHandler
{

    using IterativeDeepeningIterationCompletionHandler = std::function<void(
        StockDory::IterativeDeepeningIterationCompletionEvent
    )>;

    using IterativeDeepeningCompletionHandler = std::function<void(
        StockDory::IterativeDeepeningCompletionEvent
    )>;

    static inline IterativeDeepeningIterationCompletionHandler IterativeDeepeningIterationCompletion =
        [](const StockDory::IterativeDeepeningIterationCompletionEvent&) -> void {};

    static inline IterativeDeepeningCompletionHandler IterativeDeepeningCompletion =
        [](const StockDory::IterativeDeepeningCompletionEvent&) -> void {};

    public:
    static void RegisterIterativeDeepeningIterationCompletionHandler(
        const IterativeDeepeningIterationCompletionHandler&& handler
    )
    {
        IterativeDeepeningIterationCompletion = std::move(handler);
    }

    static void RegisterIterativeDeepeningCompletionHandler(
        const IterativeDeepeningCompletionHandler&& handler
    )
    {
        IterativeDeepeningCompletion = std::move(handler);
    }

    static void HandleIterativeDeepeningIterationCompletion(
        const StockDory::IterativeDeepeningIterationCompletionEvent& event
    )
    {
        IterativeDeepeningIterationCompletion(event);
    }

    static void HandleIterativeDeepeningCompletion(
        const StockDory::IterativeDeepeningCompletionEvent& event
    )
    {
        IterativeDeepeningCompletion(event);
    }

};

using PySearch = StockDory::ThreadedSearch<PySearchHandler>;

auto SEARCH         = PySearch();

PYBIND11_MODULE(StockDory, m)
{
    m.doc() = "Python Bindings for StockDory";

    /** -- BASE MODULES -- **/
    py::module_ core   = m.def_submodule("core"  , "Python Bindings for StockDory's Core"  );
    py::module_ engine = m.def_submodule("engine", "Python Bindings for StockDory's Engine");

    /** -- CONSTANTS -- **/
    {
        py::class_<PyConstantHolder<0>> gc (m     , "GConstant");
        py::class_<PyConstantHolder<1>> cc (core  , "CConstant");
        py::class_<PyConstantHolder<2>> ec (engine, "EConstant");

        gc.def_property_readonly_static(
            "VERSION",
            [](py::object) -> std::string { return VERSION; }
        );

        gc.def_property_readonly_static(
            "API_VERSION",
            [](py::object) -> std::string
            {
                return VERSION + strutil::to_string('+') + strutil::to_string(API_VERSION);
            }
        );

        cc.def_property_readonly_static(
            "INFINITY",
            [](py::object) -> int32_t { return StockDory::Infinity; }
        );

        ec.def_property_readonly_static(
            "MATE",
            [](py::object) -> int32_t { return StockDory::Mate; }
        );

        ec.def_property_readonly_static(
            "DRAW",
            [](py::object) -> int32_t { return StockDory::Draw; }
        );

        ec.def_property_readonly_static(
            "MAX_DEPTH",
            [](py::object) -> uint8_t { return StockDory::MaxDepth; }
        );
    }

    /** -- ENUM: core.Square -- **/
    {
        py::enum_<Square> square (core, "Square");

        for (Square sq = A1; sq <= NASQ; sq = Next(sq))
            square.value(
                strutil::capitalize(ToString(sq)).c_str(),
                sq
            );

        square.def_property_readonly(
            "next",
            [](const Square sq) -> Square
            {
                return Next(sq);
            }
        );
    }

    /** -- ENUM: core.Piece -- **/
    {
        py::enum_<Piece> piece (core, "Piece");

        for (Piece p = Pawn; p <= NAP; p = Next(p))
            piece.value(
                strutil::capitalize(ToString(p)).c_str(),
                p
            );

        piece.def_property_readonly(
            "next",
            [](const Piece p) -> Piece
            {
                return Next(p);
            }
        );
    }

    /** -- ENUM: core.Color -- **/
    {
        py::enum_<Color> color (core, "Color");

        for (Color c = White; c <= NAC; c = Next(c))
            color.value(
                strutil::capitalize(ToString(c)).c_str(),
                c
            );

        color.def_property_readonly(
            "next",
            [](const Color c) -> Color
            {
                return Next(c);
            }
        );

        color.def_property_readonly("opposite", &Opposite);
    }

    /** -- STRUCT: core.PreviousState & core.PreviousStateNull **/
    {
        py::class_<PreviousState    > previousState     (core, "PreviousState"    );
        py::class_<PreviousStateNull> previousStateNull (core, "PreviousStateNull");
    }

    /** -- CLASS: core.Board -- **/
    {
        py::class_<StockDory::Board> board (core, "Board");

        board.def(py::init());

        board.def(
            py::init<const std::string&>(),
            py::arg("fen")
        );

        board.def("__str__" , &StockDory::Board::Fen    );
        board.def("__hash__", &StockDory::Board::Zobrist);

        board.def("load_for_eval", &StockDory::Board::LoadForEvaluation);

        board.def(
            "__getitem__",
            [](const StockDory::Board& self, const Square sq) -> std::pair<Piece, Color>
            {
                const PieceColor pc = self[sq];

                return std::make_pair(pc.Piece(), pc.Color());
            },
            py::arg("sq")
        );

        board.def_property_readonly("color_to_move", &StockDory::Board::ColorToMove    );
        board.def_property_readonly("en_passant_sq", &StockDory::Board::EnPassantSquare);

        board.def(
            "king_castling_right",
            [](const StockDory::Board& self, const Color c) -> bool
            {
                if (c == White) return self.CastlingRightK<White>();
                if (c == Black) return self.CastlingRightK<Black>();

                return self.CastlingRightK<NAC>();
            },
            py::arg("color")
        );

        board.def(
            "queen_castling_right",
            [](const StockDory::Board& self, const Color c) -> bool
            {
                if (c == White) return self.CastlingRightQ<White>();
                if (c == Black) return self.CastlingRightQ<Black>();

                return self.CastlingRightQ<NAC>();
            },
            py::arg("color")
        );

        board.def(
            "checked",
            [](const StockDory::Board& self, const Color c) -> bool
            {
                if (c == White) return self.Checked<White>();
                if (c == Black) return self.Checked<Black>();

                return self.Checked<NAC>();
            },
            py::arg("color")
        );

        board.def(
            "move",
            [](StockDory::Board& self) -> PreviousStateNull
            {
                return self.Move();
            }
        );
        board.def(
            "undo_move",
            [](StockDory::Board& self, const PreviousStateNull& s) -> void
            {
                self.UndoMove(s);
            },
            py::arg("previous_state")
        );

        board.def(
            "move",
            &StockDory::Board::Move<ZOBRIST>,
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion"),
            py::arg("thread_id") = 0
        );
        board.def(
            "undo_move",
            &StockDory::Board::UndoMove<ZOBRIST>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to"),
            py::arg("thread_id") = 0
        );

        board.def(
            "move_nnue",
            &StockDory::Board::Move<ZOBRIST | NNUE>,
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion"),
            py::arg("thread_id") = 0
        );
        board.def(
            "undo_move_nnue",
            &StockDory::Board::UndoMove<ZOBRIST | NNUE>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to"),
            py::arg("thread_id") = 0
        );

        board.def(
            "move_fast",
            &StockDory::Board::Move<STANDARD>,
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion"),
            py::arg("thread_id") = 0
        );
        board.def(
            "undo_move_fast",
            &StockDory::Board::UndoMove<STANDARD>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to"),
            py::arg("thread_id") = 0
        );

        board.def(
            "move_hash",
            &StockDory::Board::Move<PERFT | ZOBRIST>,
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion"),
            py::arg("thread_id") = 0
        );
        board.def(
            "undo_move_hash",
            &StockDory::Board::UndoMove<PERFT | ZOBRIST>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to"),
            py::arg("thread_id") = 0
        );
    }

    /** -- STRUCT: core.Move & core.MoveList -- **/
    {
        py::class_<Move> move (core, "Move");

        move.def(py::init());

        move.def(
            py::init<const Square, const Square, const Piece>(),
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion")
        );

        move.def(
            py::init(
                [](const std::string& s) -> Move
                {
                    return Move::FromString(s);
                }
            ),
            py::arg("move_str")
        );

        move.def_property_readonly("from_sq"  , &Move::From     );
        move.def_property_readonly("to_sq"    , &Move::To       );
        move.def_property_readonly("promotion", &Move::Promotion);

        move.def("__eq__", &Move::operator==);

        move.def("__str__" , &Move::ToString);
        move.def("__repr__", &Move::ToString);
    }
    {
        py::class_<PyMoveList> moveList (core, "MoveList");

        moveList.def(
            py::init<const StockDory::Board&, const Piece, const Color>(),
            py::arg("board"),
            py::arg("piece"),
            py::arg("color")
        );

        moveList.def(
            py::init<const StockDory::Board&, const Square>(),
            py::arg("board"),
            py::arg("sq")
        );

        moveList.def("__getitem__", &PyMoveList::operator[]);

        moveList.def("__len__", &PyMoveList::Count);

        moveList.def(
            "__iter__",
            [](PyMoveList& self) -> py::typing::Iterator<Move&>
            {
                return py::make_iterator(self.Begin(), self.Begin() + self.Count());
            },
            py::keep_alive<0, 1>()
        );

        moveList.def("__str__" , &PyMoveList::ToString);
        moveList.def("__repr__", &PyMoveList::ToString);
    }

    /** -- MODULE: core.perft -- **/
    {
        py::module_ perft = core.def_submodule("perft", "Python Bindings for StockDory's PerftRunner");

        perft.def(
            "set_board",
            [](const std::string& s) -> void
            {
                StockDory::PerftRunner::SetBoard(s);
            },
            py::arg("fen")
        );
        perft.def(
            "set_board",
            [](const StockDory::Board& b) -> void
            {
                StockDory::PerftRunner::SetBoard(b);
            },
            py::arg("board")
        );

        perft.def(
            "perft",
            &StockDory::PerftRunner::Perft<false>,
            py::arg("depth")
        );
        perft.def(
            "perft_divide",
            &StockDory::PerftRunner::Perft<true >,
            py::arg("depth")
        );

        perft.def(
            "__perft_white",
            &StockDory::PerftRunner::Perft<White, false, false, false>,
            py::arg("board"),
            py::arg("depth")
        );
        perft.def(
            "__perft_black",
            &StockDory::PerftRunner::Perft<Black, false, false, false>,
            py::arg("board"),
            py::arg("depth")
        );

        perft.def(
            "__perft_white_divide",
            &StockDory::PerftRunner::Perft<White, true , false, false>,
            py::arg("board"),
            py::arg("depth")
        );
        perft.def(
            "__perft_black_divide",
            &StockDory::PerftRunner::Perft<Black, true , false, false>,
            py::arg("board"),
            py::arg("depth")
        );

        perft.def(
            "__perft_white_sync",
            &StockDory::PerftRunner::Perft<White, false, true , false>,
            py::arg("board"),
            py::arg("depth")
        );
        perft.def(
            "__perft_black_sync",
            &StockDory::PerftRunner::Perft<Black, false, true , false>,
            py::arg("board"),
            py::arg("depth")
        );

        perft.def(
            "__perft_white_divide_sync",
            &StockDory::PerftRunner::Perft<White, true , true , false>,
            py::arg("board"),
            py::arg("depth")
        );
        perft.def(
            "__perft_black_divide_sync",
            &StockDory::PerftRunner::Perft<Black, true , true , false>,
            py::arg("board"),
            py::arg("depth")
        );
    }

    /** -- MODULE: engine.evaluation -- **/
    {
        py::module_ evaluation = engine.def_submodule(
            "evaluation",
            "Python Bindings for StockDory's Engine Evaluation"
        );

        evaluation.def(
            "evaluate",
            [](const Color c) -> int32_t
            {
                return StockDory::Evaluation::Evaluate(c);
            },
            py::arg("perspective")
        );
    }

    /** -- MODULE: core.tp -- **/
    {
        py::module_ pool = core.def_submodule("tp", "Python Bindings for StockDory's Thread Pool");

        pool.def(
            "resize",
            [](const uint32_t n) -> void
            {
                StockDory::ThreadPool.Resize(n);
                StockDory::Evaluation::Initialize();
                SEARCH.ParallelTaskPool.Resize();
            },
            py::arg("thread_count")
        );

        pool.def(
            "size",
            [] -> uint32_t { return StockDory::ThreadPool.Size(); }
        );
    }

    /** -- MODULE: engine.tt -- **/
    {
        py::module_ tt = engine.def_submodule("tt", "Python Bindings for StockDory's Engine Transposition Table");

        tt.def(
            "resize",
            [](const size_t n) -> void { StockDory::TT.Resize(n); },
            py::arg("bytes")
        );

        tt.def(
            "size",
            [] -> size_t { return StockDory::TT.Size(); }
        );

        tt.def(
            "clear",
            [] -> void { StockDory::TT.Clear(); }
        );
    }

    /** -- STRUCT: core.VariableUCITime -- **/
    {
        py::class_<StockDory::UCITime<false>> varUCITime (core, "VariableUCITime");

        varUCITime.def(py::init());

        varUCITime.def_property(
            "white_time",
            [](const StockDory::UCITime<false>& self) -> uint64_t  { return self.WhiteTime; },
            [](StockDory::UCITime<false>& self, const uint64_t v) -> void { self.WhiteTime = v; }
        );
        varUCITime.def_property(
            "black_time",
            [](const StockDory::UCITime<false>& self) -> uint64_t  { return self.BlackTime; },
            [](StockDory::UCITime<false>& self, const uint64_t v) -> void { self.BlackTime = v; }
        );

        varUCITime.def_property(
            "white_increment",
            [](const StockDory::UCITime<false>& self) -> uint64_t  { return self.WhiteInc; },
            [](StockDory::UCITime<false>& self, const uint64_t v) -> void { self.WhiteInc = v; }
        );
        varUCITime.def_property(
            "black_increment",
            [](const StockDory::UCITime<false>& self) -> uint64_t  { return self.BlackInc; },
            [](StockDory::UCITime<false>& self, const uint64_t v) -> void { self.BlackInc = v; }
        );

        varUCITime.def_property(
            "moves_to_go",
            [](const StockDory::UCITime<false>& self) -> uint16_t  { return self.MovesToGo; },
            [](StockDory::UCITime<false>& self, const uint16_t v) -> void { self.MovesToGo = v; }
        );

        varUCITime.def_property(
            "color_to_move",
            [](const StockDory::UCITime<false>& self) -> Color { return self.ColorToMove; },
            [](StockDory::UCITime<false>& self, const Color v) -> void { self.ColorToMove = v; }
        );

        varUCITime.def(
            "as_limit",
            [](const StockDory::UCITime<false>& self, StockDory::Limit& limit) -> void
            {
                self.AsLimit(limit);
            }
        );
    }

    /** -- STRUCT: core.FixedUCITime -- **/
    {
        py::class_<StockDory::UCITime<true>> fixedUCITime (core, "FixedUCITime");

        fixedUCITime.def(py::init());

        fixedUCITime.def_property(
            "time",
            [](const StockDory::UCITime<true>& self) -> uint64_t  { return self.Time; },
            [](StockDory::UCITime<true>& self, const uint64_t v) -> void { self.Time = v; }
        );

        fixedUCITime.def(
            "as_limit",
            [](const StockDory::UCITime<true>& self, StockDory::Limit& limit) -> void
            {
                self.AsLimit(limit);
            }
        );
    }

    /** -- STRUCT: engine.MS -- **/
    {
        py::class_<StockDory::MS> ms (engine, "MS");

        ms.def(
            py::init<uint64_t>(),
            py::arg("milliseconds")
        );

        ms.def(
            "__int__",
            [](const StockDory::MS& self) -> uint64_t { return self.count(); }
        );

        ms.def(
            "__str__",
            [](const StockDory::MS& self) -> std::string { return std::to_string(self.count()) + "ms"; }
        );
    }

    /** -- STRUCT: engine.Limit -- **/
    {
        py::class_<StockDory::Limit> limit (engine, "Limit");

        limit.def(py::init());

        limit.def_property(
            "nodes",
            [](const StockDory::Limit& self) -> uint64_t { return self.Nodes; },
            [](StockDory::Limit& self, const uint64_t v) -> void { self.Nodes = v; }
        );
        limit.def_property(
            "depth",
            [](const StockDory::Limit& self) -> uint8_t { return self.Depth; },
            [](StockDory::Limit& self, const uint8_t v) -> void { self.Depth = v; }
        );
    }

    /** -- CLASS: engine.RepetitionStack -- **/
    {
        py::class_<StockDory::RepetitionStack> rs (engine, "RepetitionStack");

        rs.def(py::init());

        rs.def(
            "push" ,
            &StockDory::RepetitionStack::Push,
            py::arg("hash")
        );

        rs.def(
            "pop" ,
            &StockDory::RepetitionStack::Pop
        );

        rs.def(
            "found",
            &StockDory::RepetitionStack::Found,
            py::arg("hash"),
            py::arg("half_move_count")
        );
    }

    /** -- CLASS: engine.PrincipleVariationEntry -- **/
    {
        py::class_<StockDory::PVEntry> pv (engine, "PrincipleVariationEntry");

        pv.def(
            "__len__",
            [](const StockDory::PVEntry& self) -> size_t { return self.Ply; }
        );

        pv.def(
            "__getitem__",
            [](const StockDory::PVEntry& self, const size_t ply) -> Move { return self.PV[ply]; }
        );
    }

    /** -- MODULE: engine.event -- **/
    {
        py::module_ event = engine.def_submodule(
            "event",
            "Python Bindings for StockDory's Engine Event System"
        );

        using IDICE = StockDory::IterativeDeepeningIterationCompletionEvent;
        using IDCE  = StockDory::IterativeDeepeningCompletionEvent;

        event.def(
            "set_iterative_deepening_iteration_completion_event_handler",
            [](py::function& callback) -> void
            {
                PySearchHandler::RegisterIterativeDeepeningIterationCompletionHandler(
                    [callback = std::move(callback)](const IDICE& e) -> void
                    {
                        py::gil_scoped_acquire _;

                        // ReSharper disable once CppExpressionWithoutSideEffects
                        callback(e);
                    }
                );
            },
            py::arg("handler")
        );

        event.def(
            "set_iterative_deepening_completion_event_handler",
            [](py::function& callback) -> void
            {
                PySearchHandler::RegisterIterativeDeepeningCompletionHandler(
                    [callback = std::move(callback)](const IDCE& e) -> void
                    {
                        py::gil_scoped_acquire _;

                        // ReSharper disable once CppExpressionWithoutSideEffects
                        callback(e);
                    }
                );
            },
            py::arg("handler")
        );
    }

    /** -- MODULE: engine.search -- **/
    {
        py::module_ search = engine.def_submodule(
            "search",
            "Python Bindings for StockDory's Threaded Search System"
        );

        search.def(
            "run",
            [](StockDory::Limit& l, StockDory::Board& b, StockDory::RepetitionStack& r, const uint8_t hmc) -> void
            {
                if (SEARCH.Searching) { std::cerr << "Search is already running!" << std::endl; return; }

                SEARCH.Run(l, b, r, hmc);
            },
            py::arg("limit"),
            py::arg("board"),
            py::arg("repetition_stack"),
            py::arg("half_move_counter") = 0
        );

        search.def("stop", [] -> void { if (SEARCH.Searching) SEARCH.MainTask.Stop(); });

        search.def("is_searching", [] -> bool { return SEARCH.Searching; });
    }
}
