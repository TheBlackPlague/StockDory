//
// Copyright (c) 2025 StockDory authors. See the list of authors for more details.
// Licensed under LGPL-3.0.
//

#include <pybind11/pybind11.h>

#include "Information.h"
#include "../Backend/Board.h"
#include "../Backend/Util.h"
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

    std::array<Move, MaxMove>::iterator Begin()
    {
        return Internal.begin();
    }

};

class PyThreadPool {};
class PyTranspositionTable {};

class PySearchHandler
{

    using PV = StockDory::PrincipleVariationTable;

    using DepthIterationHandler = std::function<void(
        uint8_t ,
        uint8_t ,
         int32_t,
        uint64_t,
        uint64_t,
        StockDory::MS,
        const PV&
    )>;

    using BestMoveHandler = std::function<void(Move)>;

    static inline DepthIterationHandler DepthIterationMethod = [](const uint8_t ,
                                                                  const uint8_t ,
                                                                  const  int32_t,
                                                                  const uint64_t,
                                                                  const uint64_t,
                                                                  const StockDory::MS,
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
        DepthIterationMethod(a, b, c, d, e, f, g);
    }

    static void HandleBestMove(const Move a)
    {
              BestMoveMethod(a                  );
    }

};

using PySearch = StockDory::Search<PySearchHandler>;

auto SEARCH         = PySearch();
auto SEARCH_RUNNING = false     ;

PYBIND11_MODULE(StockDory, m)
{
    m.doc() = "Python Bindings for StockDory";

    const py::module_ types  = m.def_submodule("Types" , "Python Bindings for some of StockDory's Data Types");
    const py::module_ engine = m.def_submodule("Engine", "Python Bindings for StockDory's Engine");

    /** -- BASE -- **/
    {
        m.def("Version", [] -> std::string
        {
            return VERSION;
        });

        m.def("APIVersion", [] -> std::string
        {
            return strutil::to_string(API_VERSION);
        });
    }

    /** -- ENUM: Square -- **/
    {
        py::enum_<Square> square (types, "Square");

        for (Square sq = A1; sq <= NASQ; sq = Next(sq))
            square.value(
                strutil::capitalize(StockDory::Util::SquareToString(sq)).c_str(),
                sq
            );

        square.def("Next", [](const Square sq) -> Square
        {
            return Next(sq);
        }, "Get the next square in the sequence. No wrap.");
    }

    /** -- ENUM: Piece -- **/
    {
        py::enum_<Piece> piece (types, "Piece");

        for (Piece p = Pawn; p <= NAP; p = Next(p))
            piece.value(
                strutil::capitalize(ToString(p)).c_str(),
                p
            );

        piece.def("Next", [](const Piece p) -> Piece
        {
            return Next(p);
        }, "Get the next piece in the sequence. No wrap.");
    }

    /** -- ENUM: Color -- **/
    {
        py::enum_<Color> color (types, "Color");

        for (Color c = White; c <= NAC; c = Next(c))
            color.value(
                strutil::capitalize(ToString(c)).c_str(),
                c
            );

        color.def("Next", [](const Color c) -> Color
        {
            return Next(c);
        }, "Get the next color in the sequence. No wrap.");

        color.def("Opposite", &Opposite, "Get the opposite color of this color.");
    }

    /** -- STRUCT: PreviousState & PreviousStateNull **/
    {
        py::class_<PreviousState    > previousState     (types, "PreviousState"    );
        py::class_<PreviousStateNull> previousStateNull (types, "PreviousStateNull");
    }

    /** -- CLASS: Board -- **/
    {
        py::class_<StockDory::Board> board(m, "Board");

        board.def(py::init());

        board.def(py::init<const std::string&>(), py::arg("fen"));

        board.def("LoadForEvaluation", &StockDory::Board::LoadForEvaluation,
                  "Load the board into the evaluation neural network.");

        board.def("Fen", &StockDory::Board::Fen, "Get the FEN for the board.");

        board.def("Zobrist", &StockDory::Board::Zobrist,
                  "Get the Zobrist Hash for the board.");

        board.def("__getitem__", [](const StockDory::Board& self, const Square sq) -> std::pair<Piece, Color>
        {
            const PieceColor pc = self[sq];

            return std::make_pair(pc.Piece(), pc.Color());
        }, "Get the piece and color of the piece at a particular square on the board.");

        board.def("ColorToMove", &StockDory::Board::ColorToMove, "Get the color to move.");

        board.def("CastlingRightK", [](const StockDory::Board& self, const Color c) -> bool
        {
            if (c == White) return self.CastlingRightK<White>();
            if (c == Black) return self.CastlingRightK<Black>();

            return self.CastlingRightK<NAC>();
        }, "Get the king-sided castling rights.");

        board.def("CastlingRightQ", [](const StockDory::Board& self, const Color c) -> bool
        {
            if (c == White) return self.CastlingRightQ<White>();
            if (c == Black) return self.CastlingRightQ<Black>();

            return self.CastlingRightQ<NAC>();
        }, "Get the queen-sided castling rights.");

        board.def("EnPassant", &StockDory::Board::EnPassantSquare, "Get the En Passant square.");

        board.def("Checked", [](const StockDory::Board& self, const Color c) -> bool
        {
            if (c == White) return self.Checked<White>();
            if (c == Black) return self.Checked<Black>();

            return self.Checked<NAC>();
        }, "Returns true if the color is under check.");

        board.def("Move", [](StockDory::Board& self) -> PreviousStateNull
        {
            return self.Move();
        }, "Make a null move.");

        board.def("Move"    , &StockDory::Board::Move<ZOBRIST        >, "Make a regular move.");
        board.def("MoveNNUE", &StockDory::Board::Move<ZOBRIST | NNUE >,
                  "Make a regular move and update the neural network.");
        board.def("MoveFAST", &StockDory::Board::Move<STANDARD       >,
                  "Make a regular move ignoring hashing.");
        board.def("MoveHASH", &StockDory::Board::Move<PERFT | ZOBRIST>,
                  "Make a regular move and minimize hash collisions.");

        board.def("UndoMove", [](StockDory::Board& self, const PreviousStateNull& state) -> void
        {
            self.UndoMove(state);
        }, "Undo a null move.");

        board.def("UndoMove"    , &StockDory::Board::UndoMove<ZOBRIST        >, "Undo a regular move.");
        board.def("UndoMoveNNUE", &StockDory::Board::UndoMove<ZOBRIST | NNUE >,
                  "Undo a regular move and update the neural network.");
        board.def("UndoMoveFAST", &StockDory::Board::UndoMove<STANDARD       >,
                  "Undo a regular move ignoring hashing.");
        board.def("UndoMoveHASH", &StockDory::Board::UndoMove<PERFT | ZOBRIST>,
                  "Undo a regular move and minimize hash collisions.");
    }

    /** -- STRUCT: Move & MoveList -- **/
    {
        py::class_<Move> move (types, "Move");

        move.def(py::init());

        move.def(
            py::init<const Square, const Square, const Piece>(),
            py::arg("from"), py::arg("to"), py::arg("promotion")
        );

        move.def(py::init([](const std::string& moveStr)
        {
            return Move::FromString(moveStr);
        }));

        move.def("From", &Move::From, "Get the from square.");

        move.def("To", &Move::To, "Get the to square.");

        move.def("Promotion", &Move::Promotion, "Get the promotion piece.");

        move.def("__eq__", &Move::operator==);

        move.def("__str__", &Move::ToString);
    }
    {
        py::class_<PyMoveList> moveList (types, "MoveList");

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

        moveList.def("__iter__", [](PyMoveList& self) -> py::typing::Iterator<Move&>
        {
            return py::make_iterator(self.Begin(), self.Begin() + self.Count());
        }, py::keep_alive<0, 1>());

        moveList.def("__str__", [](const PyMoveList& self) -> std::string
        {
            std::stringstream ss;
            ss << "[";

            const size_t n = self.Count();
            for (size_t i = 0; i < n; i++) {
                ss << self[i].ToString();

                if (i != n - 1) ss << ", ";
            }

            ss << "]";

            return ss.str();
        });
    }

    /** -- CLASS: PERFT -- **/
    {
        py::class_<StockDory::PerftRunner> perft (engine, "PerftDriver");

        perft.def_static("SetBoard", [](const std::string& fen) -> void
        {
            StockDory::PerftRunner::SetBoard(fen);
        }, "Set the board for PERFT.");

        perft.def_static("SetBoard", [](const StockDory::Board& board) -> void
        {
            StockDory::PerftRunner::SetBoard(board);
        }, "Set the board for PERFT.");

        perft.def_static("Perft" , &StockDory::PerftRunner::Perft<false>, "Run PERFT to a certain depth.");
        perft.def_static("PerftD", &StockDory::PerftRunner::Perft<true >, "Run PERFT to a certain depth.");

        perft.def_static("_WPerft" , &StockDory::PerftRunner::Perft<White, false, false, false>,
            "Internal PERFT method assuming white is side to move.");
        perft.def_static("_BPerft" , &StockDory::PerftRunner::Perft<Black, false, false, false>,
            "Internal PERFT method assuming black is side to move.");
        perft.def_static("_WPerftD", &StockDory::PerftRunner::Perft<White, true , false, false>,
            "Internal PERFT method assuming white is side to move.");
        perft.def_static("_BPerftD", &StockDory::PerftRunner::Perft<Black, true , false, false>,
            "Internal PERFT method assuming black is side to move.");

        perft.def_static("_WPerft_Sync" , &StockDory::PerftRunner::Perft<White, false, true, false>,
            "Internal PERFT method assuming white is side to move. This will force synchronous computation.");
        perft.def_static("_BPerft_Sync" , &StockDory::PerftRunner::Perft<Black, false, true, false>,
            "Internal PERFT method assuming black is side to move. This will force synchronous computation.");
        perft.def_static("_WPerftD_Sync", &StockDory::PerftRunner::Perft<White, true , true, false>,
            "Internal PERFT method assuming white is side to move. This will force synchronous computation.");
        perft.def_static("_BPerftD_Sync", &StockDory::PerftRunner::Perft<Black, true , true, false>,
            "Internal PERFT method assuming black is side to move. This will force synchronous computation.");
    }

    /** -- CLASS: Evaluation -- **/
    {
        py::class_<StockDory::Evaluation> evaluation (engine, "Evaluation");

        evaluation.def_static("Evaluate", [](const Color c) -> int32_t
        {
            return StockDory::Evaluation::Evaluate(c);
        }, "Evaluate the currently loaded board from a color's point of view.");
    }

    /** -- CLASS: ThreadPool -- **/
    {
        py::class_<PyThreadPool> pool (engine, "ThreadPool");

        pool.def_static("Resize", [](const size_t c) -> void
        {
            pool_set_size(
                StockDory::ThreadPool,
                std::max<size_t>(1, std::min<size_t>(core_count(), c))
            );
        }, "Resize the thread pool. Minimum 1 thread, maximum is the perceived logical processor count.");

        pool.def_property_readonly_static("Count", [] -> size_t
        {
            return pool_size(StockDory::ThreadPool);
        }, "Size of the thread pool.");
    }

    /** -- CLASS: TranspositionTable -- **/
    {
        py::class_<PyTranspositionTable> tt (engine, "TranspositionTable");

        tt.def_static("Resize", [](const size_t c) -> void
        {
            TTable.Resize(c);
        });

        tt.def_static("Size", [] -> size_t
        {
            return TTable.Size();
        });

        tt.def_static("Clear", [] -> void
        {
            TTable.Clear();
        });
    }

    /** -- CLASS: MS -- **/
    {
        py::class_<StockDory::MS> ms (types, "MS");

        ms.def("Get", &StockDory::MS::count);
    }

    /** -- STRUCT: TimeData -- **/
    {
        py::class_<StockDory::TimeData> td (engine, "TimeData");

        td.def(py::init());

        td.def_property(
            "WhiteTime",
            [](const StockDory::TimeData& self) -> uint64_t
            {
                return self.WhiteTime;
            },
            [](StockDory::TimeData& self, const uint64_t whiteTime) -> void
            {
                self.WhiteTime = whiteTime;
            }
        );

        td.def_property(
            "BlackTime",
            [](const StockDory::TimeData& self) -> uint64_t
            {
                return self.BlackTime;
            },
            [](StockDory::TimeData& self, const uint64_t blackTime) -> void
            {
                self.BlackTime = blackTime;
            }
        );

        td.def_property(
            "WhiteIncrement",
            [](const StockDory::TimeData& self) -> uint64_t
            {
                return self.WhiteIncrement;
            },
            [](StockDory::TimeData& self, const uint64_t whiteIncrement) -> void
            {
                self.WhiteIncrement = whiteIncrement;
            }
        );

        td.def_property(
            "BlackIncrement",
            [](const StockDory::TimeData& self) -> uint64_t
            {
                return self.BlackIncrement;
            },
            [](StockDory::TimeData& self, const uint64_t blackIncrement) -> void
            {
                self.BlackIncrement = blackIncrement;
            }
        );

        td.def_property(
            "MovesToGo",
            [](const StockDory::TimeData& self) -> uint16_t
            {
                return self.MovesToGo;
            },
            [](StockDory::TimeData& self, const uint16_t movesToGo) -> void
            {
                self.MovesToGo = movesToGo;
            }
        );
    }

    /** -- CLASS: TimeControl & TimeManager -- **/
    {
        py::class_<StockDory::TimeControl> tc (engine, "TimeControl");
    }
    {
        py::class_<StockDory::TimeManager> tm (engine, "TimeManager");

        tm.def_static("Default", &StockDory::TimeManager::Default);
        tm.def_static("Fixed"  , &StockDory::TimeManager::Fixed  );
        tm.def_static("Optimal", &StockDory::TimeManager::Optimal);
    }

    /** -- CLASS: RepetitionHistory -- **/
    {
        py::class_<StockDory::RepetitionHistory> rh (engine, "RepetitionHistory");

        rh.def(py::init<const ZobristHash>(), py::arg("hash"));

        rh.def("Push" , &StockDory::RepetitionHistory::Push );
        rh.def("Pull" , &StockDory::RepetitionHistory::Pull );
        rh.def("Found", &StockDory::RepetitionHistory::Found);
    }

    /** -- CLASS: PrincipleVariationTable -- **/
    {
        py::class_<StockDory::PrincipleVariationTable> pv (engine, "PrincipleVariationTable");

        pv.def("__len__", &StockDory::PrincipleVariationTable::Count);

        pv.def("__getitem__", &StockDory::PrincipleVariationTable::operator[]);
    }

    /** -- CLASS: SearchHandler -- **/
    {
        py::class_<PySearchHandler> handler (engine, "SearchHandler");

        handler.def_static(
            "RegisterDepthIterationHandler",
            [](py::function& callback) -> void
            {
                PySearchHandler::RegisterDepthIterationHandler(
                    [callback = std::move(callback)](const uint8_t       a, const uint8_t  b, const int32_t c,
                                                      const uint64_t      d, const uint64_t e,
                                                      const StockDory::MS f,
                                                      const StockDory::PrincipleVariationTable& g) -> void
                    {
                        py::gil_scoped_acquire _;

                        // ReSharper disable once CppExpressionWithoutSideEffects
                        callback(a, b, c, d, e, f, g);
                    }
                );
            },
            py::arg("handler"),
            R"pbdoc(
                RegisterDepthIterationHandler(handler: Callable[[int, int, int, int, int, StockDory.Types.MS, StockDory.Engine.PrincipleVariationTable], None]) -> None

                Register a handler to be called when search has concluded a depth iteration.
            )pbdoc"
        );

        handler.def_static(
                  "RegisterBestMoveHandler",
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
            py::arg("handler"),
            R"pbdoc(
                RegisterBestMoveHandler(handler: Callable[[StockDory.Types.Move], None]) -> None

                Register a handler to be called when search has concluded to have found the best move.
            )pbdoc"
        );
    }

    /** -- STRUCT: SearchLimit -- **/
    {
        py::class_<StockDory::Limit> limit (engine, "SearchLimit");

        limit.def(py::init());

        limit.def(py::init<uint64_t>(), py::arg("nodes"));

        limit.def(py::init<uint8_t >(), py::arg("depth"));
    }

    /** -- CLASS: Search -- **/
    {
        py::class_<PySearch> search (engine, "Search");

        search.def_static(
            "Configure",
            [](const StockDory::Board&                  board, const StockDory::TimeControl& tc,
               const StockDory::RepetitionHistory& repetition, const uint8_t                 hm) -> void
            {
                SEARCH = PySearch(board, tc, repetition, hm);
            },
            py::arg("board"),
            py::arg("time_control"),
            py::arg("repetition_history"),
            py::arg("half_move_count"),
            R"pbdoc(
                Configure(board: StockDory.Board, time_control: StockDory.Engine.TimeControl, repetition_history: StockDory.Engine.RepetitionHistory, hm: int) -> None

                Configures the search.
            )pbdoc"
        );

        search.def_static(
            "Start",
            [](const StockDory::Limit& limit = StockDory::Limit()) -> void
            {
                drjit::do_async([limit] -> void
                {
                    SEARCH_RUNNING = true ;
                    SEARCH.IterativeDeepening(limit);
                    SEARCH_RUNNING = false;
                }, {}, StockDory::ThreadPool);
            },
            py::arg("limit") = StockDory::Limit(),
            R"pbdoc(
                Run(limit: StockDory.Engine.SearchLimit = StockDory.Engine.SearchLimit()) -> None

                Starts the asynchronous search using the provided search limit constraints,
                using the default constraints if none are provided.
            )pbdoc"
        );

        search.def_static(
            "Stop",
            [] -> void
            {
                SEARCH.ForceStop();
            },
            R"pbdoc(
                Stop() -> None

                Gracefully halts the ongoing search if it's running.
            )pbdoc"
        );

        search.def_static(
            "Running",
            []() -> bool { return SEARCH_RUNNING; },
            R"pbdoc(
                Running() -> bool

                Returns true if a search is running, otherwise false.
            )pbdoc"
        );
    }
}
