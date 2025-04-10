//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <pybind11/pybind11.h>

#include "Information.h"
#include "../Backend/Board.h"
#include "../Backend/Move/MoveList.h"
#include "../Backend/Type/Color.h"
#include "../Backend/Type/Move.h"
#include "../Backend/Type/Piece.h"
#include "../Backend/Type/Square.h"
#include "../Engine/EngineParameter.h"
#include "../Engine/RepetitionHistory.h"
#include "../Engine/Search.h"
#include "../Engine/Move/PrincipleVariationTable.h"
#include "../Engine/Time/TimeManager.h"
#include "../External/strutil.h"
#include "../Terminal/Perft/PerftRunner.h"

namespace py = pybind11;

constexpr size_t API_VERSION = 0;

class PyMoveList
{

    std::array<Move, MaxMove> Internal = {};

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

    std::array<Move, MaxMove>::iterator Begin()
    {
        return Internal.begin();
    }

};

template<size_t T>
class PyConstantHolder {};

class PySearchHandler
{

    using PV = StockDory::PrincipleVariationTable;

    using DepthIterationHandler = std::function<void(
        uint8_t ,
        uint8_t ,
         int32_t,
        uint64_t,
        uint64_t,
         int64_t,
        const PV&
    )>;

    using BestMoveHandler = std::function<void(Move)>;

    static inline DepthIterationHandler DepthIterationMethod = [](const uint8_t ,
                                                                  const uint8_t ,
                                                                  const  int32_t,
                                                                  const uint64_t,
                                                                  const uint64_t,
                                                                  const  int64_t,
                                                                  const PV& ) -> void {};
    static inline       BestMoveHandler       BestMoveMethod = [](const Move) -> void {};

    public:
    static void RegisterDepthIterationHandler(const DepthIterationHandler&& handler)
    {
        DepthIterationMethod = std::move(handler);
    }

    static void       RegisterBestMoveHandler(const       BestMoveHandler&& handler)
    {
              BestMoveMethod = std::move(handler);
    }

    static void HandleDepthIteration(const uint8_t       a, const uint8_t  b, const int32_t c,
                                     const uint64_t      d, const uint64_t e,
                                     const StockDory::MS f, const PV&      g)
    {
        DepthIterationMethod(a, b, c, d, e, f.count(), g);
    }

    static void HandleBestMove(const Move a)
    {
              BestMoveMethod(a                          );
    }

};

using PySearch = StockDory::Search<PySearchHandler>;

auto SEARCH         = PySearch();
auto SEARCH_RUNNING = false     ;

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
            [](py::object) -> int32_t { return Infinity; }
        );

        ec.def_property_readonly_static(
            "MATE",
            [](py::object) -> int32_t { return Mate; }
        );

        ec.def_property_readonly_static(
            "DRAW",
            [](py::object) -> int32_t { return Draw; }
        );

        ec.def_property_readonly_static(
            "MAX_DEPTH",
            [](py::object) -> uint8_t { return MaxDepth; }
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
            py::arg("promotion")
        );
        board.def(
            "undo_move",
            &StockDory::Board::UndoMove<ZOBRIST>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to")
        );

        board.def(
            "move_nnue",
            &StockDory::Board::Move<ZOBRIST | NNUE>,
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion")
        );
        board.def(
            "undo_move_nnue",
            &StockDory::Board::UndoMove<ZOBRIST | NNUE>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to")
        );

        board.def(
            "move_fast",
            &StockDory::Board::Move<STANDARD>,
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion")
        );
        board.def(
            "undo_move_fast",
            &StockDory::Board::UndoMove<STANDARD>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to")
        );

        board.def(
            "move_hash",
            &StockDory::Board::Move<PERFT | ZOBRIST>,
            py::arg("from"),
            py::arg("to"),
            py::arg("promotion")
        );
        board.def(
            "undo_move_hash",
            &StockDory::Board::UndoMove<PERFT | ZOBRIST>,
            py::arg("previous_state"),
            py::arg("from"),
            py::arg("to")
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

        move.def_property_readonly("from"     , &Move::From     );
        move.def_property_readonly("to"       , &Move::To       );
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
            [](const uint32_t n) -> void { StockDory::ThreadPool.Resize(n); },
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
            [](const size_t n) -> void { TTable.Resize(n); },
            py::arg("bytes")
        );

        tt.def(
            "size",
            [] -> size_t { return TTable.Size(); }
        );

        tt.def(
            "clear",
            [] -> void { TTable.Clear(); }
        );
    }

    /** -- STRUCT: core.TimeData -- **/
    {
        py::class_<StockDory::TimeData> td (core, "TimeData");

        td.def(py::init());

        td.def_property(
            "white_time",
            [](const StockDory::TimeData& self) -> uint64_t  { return self.WhiteTime; },
            [](StockDory::TimeData& self, const uint64_t v) -> void { self.WhiteTime = v; }
        );
        td.def_property(
            "black_time",
            [](const StockDory::TimeData& self) -> uint64_t  { return self.BlackTime; },
            [](StockDory::TimeData& self, const uint64_t v) -> void { self.BlackTime = v; }
        );

        td.def_property(
            "white_increment",
            [](const StockDory::TimeData& self) -> uint64_t  { return self.WhiteIncrement; },
            [](StockDory::TimeData& self, const uint64_t v) -> void { self.WhiteIncrement = v; }
        );
        td.def_property(
            "black_increment",
            [](const StockDory::TimeData& self) -> uint64_t  { return self.BlackIncrement; },
            [](StockDory::TimeData& self, const uint64_t v) -> void { self.BlackIncrement = v; }
        );

        td.def_property(
            "moves_to_go",
            [](const StockDory::TimeData& self) -> uint16_t  { return self.MovesToGo; },
            [](StockDory::TimeData& self, const uint16_t v) -> void { self.MovesToGo = v; }
        );
    }

    /** -- CLASS: engine.TimeControl -- **/
    {
        py::class_<StockDory::TimeControl> tc (engine, "TimeControl");
    }

    /** -- MODULE: engine.tm -- **/
    {
        py::module_ tm = engine.def_submodule("tm", "Python Bindings for StockDory's Engine Time Manager");

        tm.def(
            "default",
            &StockDory::TimeManager::Default
        );

        tm.def(
            "fixed"  ,
            &StockDory::TimeManager::Fixed,
            py::arg("milliseconds")
        );

        tm.def(
            "optimal",
            &StockDory::TimeManager::Optimal,
            py::arg("board"),
            py::arg("time_data")
        );
    }

    /** -- CLASS: engine.RepetitionHistory -- **/
    {
        py::class_<StockDory::RepetitionHistory> rh (engine, "RepetitionHistory");

        rh.def(
            py::init<const ZobristHash>(),
            py::arg("hash")
        );

        rh.def(
            "push" ,
            &StockDory::RepetitionHistory::Push,
            py::arg("hash")
        );

        rh.def(
            "pull" ,
            &StockDory::RepetitionHistory::Pull
        );

        rh.def(
            "found",
            &StockDory::RepetitionHistory::Found,
            py::arg("hash"),
            py::arg("half_move_count")
        );
    }

    /** -- CLASS: engine.PrincipleVariationTable -- **/
    {
        py::class_<StockDory::PrincipleVariationTable> pv (engine, "PrincipleVariationTable");

        pv.def("__len__", &StockDory::PrincipleVariationTable::Count);

        pv.def("__getitem__", &StockDory::PrincipleVariationTable::operator[]);
    }

    /** -- MODULE: engine.event -- **/
    {
        py::module_ event = engine.def_submodule(
            "event",
            "Python Bindings for StockDory's Engine Event System"
        );

        event.def(
            "set_depth_iteration_event_handler",
            [](py::function& callback) -> void
            {
                PySearchHandler::RegisterDepthIterationHandler(
                    [callback = std::move(callback)](const uint8_t       a, const uint8_t  b, const int32_t c,
                                                      const uint64_t      d, const uint64_t e,
                                                      const int64_t f,
                                                      const StockDory::PrincipleVariationTable& g) -> void
                    {
                        py::gil_scoped_acquire _;

                        // ReSharper disable once CppExpressionWithoutSideEffects
                        callback(a, b, c, d, e, f, g);
                    }
                );
            },
            py::arg("handler")
        );

        event.def(
            "set_best_move_event_handler",
            [](py::function& callback) -> void
            {
                PySearchHandler::     RegisterBestMoveHandler(
                    [callback = std::move(callback)](const Move a) -> void
                    {
                        py::gil_scoped_acquire _;

                        // ReSharper disable once CppExpressionWithoutSideEffects
                        callback(a                  );
                    }
                );
            },
            py::arg("handler")
        );
    }

    /** -- STRUCT: engine.Limit -- **/
    {
        py::class_<StockDory::Limit> limit (engine, "Limit");

        limit.def(py::init());

        limit.def(py::init<uint64_t>(), py::arg("nodes"));

        limit.def(py::init<uint8_t >(), py::arg("depth"));
    }

    /** -- MODULE: engine.search -- **/
    {
        py::module_ search = engine.def_submodule(
            "search",
            "Python Bindings for StockDory's Asynchronous Search System"
        );

        search.def(
            "configure",
            [](const StockDory::Board&             a, const StockDory::TimeControl& b,
                const StockDory::RepetitionHistory& c, const uint8_t                 d) -> void
            {
                SEARCH = PySearch(a, b, c, d);
            },
            py::arg("board"),
            py::arg("time_control"),
            py::arg("repetition_history"),
            py::arg("half_move_count")
        );

        search.def(
            "start_async",
            [](const StockDory::Limit& limit = StockDory::Limit()) -> void
            {
                drjit::do_async([limit] -> void
                {
                    SEARCH_RUNNING = true ;
                    SEARCH.IterativeDeepening(limit);
                    SEARCH_RUNNING = false;
                }, {}, ~StockDory::ThreadPool);
            },
            py::arg("limit") = StockDory::Limit()
        );

        search.def("stop_async", [] -> void {        SEARCH.ForceStop(); });

        search.def("is_running", [] -> bool { return SEARCH_RUNNING;     });
    }
}
