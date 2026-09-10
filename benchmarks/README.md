# Backend validation

The correctness runner uses the production `PerftRunner`. Its default synchronous
run checks every published node count through depth 7 from the
[Chess Programming Wiki](https://www.chessprogramming.org/Perft_Results), including
the mirrored fourth position. It checks the restored FEN and Zobrist hash after
each traversal and exits unsuccessfully if any check fails. Position 2 and both
versions of position 4 have published results through depth 6; position 5 has
published results through depth 5. There are 51 checks in the complete suite.

The benchmark uses [nanobench 4.3.11](https://github.com/martinus/nanobench/releases/tag/v4.3.11)
to time the same production PERFT function inside the process. Board construction,
output, and board restoration checks occur outside the timed region. Every timed
invocation checks its node count. The final-ply move-counting optimization remains
enabled, matching StockDory's actual PERFT command.

## Build

First configure and build StockDory with Ninja, LLVM 22, `CMAKE_BUILD_TYPE=Release`,
`BUILD_NATIVE=ON`, and `BUILD_PGO=OFF`. Obtain the unmodified
[nanobench header](https://raw.githubusercontent.com/martinus/nanobench/v4.3.11/src/include/nanobench.h)
in a directory outside the repository. Then run:

```sh
python3 benchmarks/build.py \
    --engine-build /absolute/path/to/engine-build \
    --source /absolute/path/to/StockDory \
    --output /absolute/path/to/validation/candidate \
    --nanobench /absolute/path/to/nanobench.h \
    --board-type StockDory::PerftBoard
```

The build script reuses the actual engine's compiler, include directories, release
options, native architecture options, linker, and dependencies. It rejects PGO or
instrumented builds and writes the commands and binary hashes to
`build-manifest.json`. It does not modify the engine build's objects.

For the frozen pre-rewrite worktree, use its own engine build and source directory,
set `--board-type StockDory::Board`, and add `--baseline`. That compatibility flag
handles depth zero in the runner because the old PERFT entry point cannot accept
zero. The rewritten entry point is tested directly, including depth zero.

## Correctness

```sh
/absolute/path/to/validation/candidate/BackendPerft > perft.jsonl
/absolute/path/to/validation/candidate/BackendPerft --threads 4 > perft-parallel.jsonl
/absolute/path/to/validation/candidate/MoveFlags
/absolute/path/to/validation/candidate/BackendState
```

The default is the complete suite. For iteration only, `--max-depth 5` limits the
depth or `--position kiwipete` selects a single position. These options do not count
unexecuted cases as passed. Use all positions at the default maximum depth for the
final correctness gate.
`--threads 4` exercises the production parallel implementation and board copying
with the same complete set of expected node counts.

`MoveFlags` exhaustively checks every legal flag over every distinct pair of
squares, promotion decoding, UCI identity round trips, full equality versus move
identity, null moves, and malformed UCI inputs.

`BackendState` checks deterministic legal playouts, all profile conversion
directions, incremental hashes against reconstruction from FEN, mailbox and
bitboard consistency, generated move flags, make/undo and null-move restoration,
and incremental NNUE scores against a separate freshly rebuilt accumulator for
both colors. Extra positions exercise castling, each promotion, en passant and
illegal en passant exposing the king.

Capture the production engine's `bench` output before and after the rewrite as
well. Identical search node totals provide a separate search-behavior check; a
PERFT pass alone does not establish identical search behavior.

## Performance

Run on an otherwise idle machine. Choose one allowed logical CPU and keep its
sibling idle. Do not run builds, deep correctness traversals, or other benchmarks
alongside this comparison.

```sh
python3 benchmarks/compare.py \
    --baseline /absolute/path/to/validation/baseline/BackendPerftBenchmark \
    --candidate /absolute/path/to/validation/candidate/BackendPerftBenchmark \
    --output /absolute/path/to/validation/comparison \
    --cpu 4 --pairs 31
```

The script pins itself and both executables to the selected CPU. It randomizes
position order and baseline/candidate order within each pair. Every process runs
two warmup traversals and a timed batch of 16 traversals. The default collects 31 independent
pairs per position; fewer than 21 pairs are rejected. No subprocess startup or
output time enters the measurements.

The two warmups run explicitly before entering nanobench. Its built-in warmup is
disabled because version 4.3.11 recalibrates the first epoch after warmup even when
a fixed iteration count is configured; the comparator rejects an unexpected
iteration count.

`--iterations` controls the number of traversals in each timed batch. Longer
batches average scheduling noise on shared machines. Nanobench reports elapsed
time normalized per traversal; the script verifies the requested iteration count
and does not divide this already-normalized value a second time.

All nanobench samples, pair order, binary hashes, CPU information, and raw paired
measurements are retained. The report estimates candidate/baseline runtime ratios
using paired log ratios with 50,000 bootstrap resamples. The confidence intervals
use a Bonferroni correction across the seven workloads for simultaneous 95%
coverage. An upper limit below 1 demonstrates improvement on that workload. A
lower limit above 1 demonstrates a regression. An interval spanning 1 is reported
as inconclusive, and the strict gate does not pass it as "no regression."

For an unresolved workload, `--position position-5 --iterations 64` can collect a
fresh focused confirmation in a new output directory. The confidence correction
always retains at least the original seven comparisons; use
`--confidence-family-size 14` to make that additional look more conservative.
Preserve the earlier samples and identify the differing batch size in the final
report.

Statistical evidence applies to these workloads and this machine; it cannot prove
an absence of regressions on every processor or every possible position.
