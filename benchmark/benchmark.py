#
# Copyright (c) 2026 Shaheryar Sohail and Lee Durbin
# SPDX-License-Identifier: AGPL-3.0-only
#

"""
Compares two StockDory binaries using their built-in single-threaded bench.

Requires Python 3.10+ and no third-party packages. Example:
    python3 benchmark/benchmark.py ./StockDory-base ./StockDory-test --runs 30
"""

import argparse
import math
import platform
import re
import statistics
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


SUMMARY = re.compile(r"([0-9]+)\s+nodes\s+([0-9]+)\s+nps")


@dataclass(frozen=True)
class BenchResult:
    nodes: int
    nps: int


def parse_result(output: str) -> BenchResult:
    matches = [match for line in output.splitlines() if (match := SUMMARY.fullmatch(line.strip()))]

    if len(matches) != 1: raise ValueError("expected exactly one '<nodes> nodes <nps> nps' summary")

    match = matches[0]
    assert match is not None, "match should not be None"

    nodes, nps = map(int, match.groups())

    if nodes <= 0 or nps <= 0: raise ValueError("bench must report positive nodes and NPS")

    return BenchResult(nodes, nps)


def run_bench(executable: Path, timeout: float) -> BenchResult:
    try:
        with subprocess.Popen(
            [str(executable), "bench"],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding="utf-8",
            errors="replace",
        ) as process:
            try:
                output, _ = process.communicate(timeout=timeout)
            except (subprocess.TimeoutExpired, KeyboardInterrupt):
                process.kill()
                process.communicate()
                raise
    except subprocess.TimeoutExpired as timeout_error:
        raise RuntimeError(f"{executable}: bench timed out after {timeout:g}s") from timeout_error
    if process.returncode != 0:
        raise RuntimeError(
            f"{executable}: bench exited with code {process.returncode}\n"
            f"{output[-2000:]}"
        )

    try:
        return parse_result(output)
    except ValueError as timeout_error:
        raise RuntimeError(f"{executable}: {timeout_error}\n{output[-2000:]}") from timeout_error


def t_critical_95(degrees: int) -> float:
    if degrees < 19: raise ValueError("at least 20 measured pairs are required")

    z = statistics.NormalDist().inv_cdf(0.975)
    return (
        z
        + (z**3 + z) / (4 * degrees)
        + (5 * z**5 + 16 * z**3 + 3 * z) / (96 * degrees**2)
        + (3 * z**7 + 19 * z**5 + 17 * z**3 - 15 * z) / (384 * degrees**3)
        + (79 * z**9 + 776 * z**7 + 1482 * z**5 - 1920 * z**3 - 945 * z)
        / (92160 * degrees**4)
    )


def compare(pairs: list[tuple[int, int]]) -> tuple[float, float, float]:
    critical = t_critical_95(len(pairs) - 1)

    ratios = [math.log(candidate / baseline) for baseline, candidate in pairs]
    center = statistics.fmean(ratios)
    margin = critical * statistics.stdev(ratios) / math.sqrt(len(ratios))

    return (100 * math.expm1(center),
            100 * math.expm1(center - margin),
            100 * math.expm1(center + margin))


def verdict(lower: float, upper: float) -> str:
    if lower == upper: return "inconclusive (no measured variation; check timer resolution)"

    if lower > 0: return "statistically significant speedup"
    if upper < 0: return "statistically significant slowdown"

    return "inconclusive (95% CI includes zero)"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Measure a non-functional StockDory patch's speedup.",
        epilog=("Runs the existing 'bench' command with its fixed settings. "
                "Stop other workloads and use matching compiler/build flags. "
                "Exit status: 0 completed, 1 benchmark failure, 2 usage error, "
                "130 interrupted; significance is reported separately."),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("baseline", type=Path, help="baseline executable path")
    parser.add_argument("candidate", type=Path, help="candidate executable path")

    parser.add_argument("--runs", type=int, default=30,
                        help="measured runs per binary, at least 20")
    parser.add_argument("--warmup", type=int, default=2,
                        help="discarded warmup runs per binary")
    parser.add_argument("--timeout", type=float, default=300,
                        help="maximum seconds per bench process")

    args = parser.parse_args()

    if args.runs < 20: parser.error("--runs must be at least 20")

    if args.warmup < 0: parser.error("--warmup must be non-negative")

    if not math.isfinite(args.timeout) or args.timeout <= 0: parser.error("--timeout must be finite and positive")

    for name in ("baseline", "candidate"):
        path = getattr(args, name).expanduser().resolve()

        if not path.is_file(): parser.error(f"{name} executable is not a file: {path}")

        setattr(args, name, path)

    return args


def main() -> int:
    args = parse_args()

    binaries = (args.baseline, args.candidate)

    expected_nodes = None

    pairs: list[tuple[int, int]] = []

    print("StockDory speed benchmark")

    print(f"Platform:  {platform.platform()} / {platform.machine()}")

    print(f"Baseline:  {args.baseline}")
    print(f"Candidate: {args.candidate}")

    print(f"Runs: {args.runs} pairs; warmup: {args.warmup} pairs; timeout: {args.timeout:g}s per process")

    print("Sequential bench runs, alternating baseline/candidate order.\n", flush=True)

    for phase, count in (("Warmup", args.warmup), ("Run", args.runs)):
        for index in range(count):
            order = (0, 1) if index % 2 == 0 else (1, 0)
            samples = [0, 0]

            for side in order:
                label = "baseline" if side == 0 else "candidate"

                print(f"{phase} {index + 1}/{count}: {label}", file=sys.stderr, flush=True)

                result = run_bench(binaries[side], args.timeout)

                if expected_nodes is None: expected_nodes = result.nodes

                elif result.nodes != expected_nodes:
                    raise RuntimeError(
                        f"{phase} {index + 1}, {label}: node count changed "
                        f"({expected_nodes:,} -> {result.nodes:,}). "
                        "Not a comparable non-functional benchmark; no conclusion."
                    )

                samples[side] = result.nps

            if phase == "Run":
                pairs.append((samples[0], samples[1]))
                print(
                    f"{index + 1:3d}/{count}: baseline {samples[0]:>12,} NPS | candidate {samples[1]:>12,} NPS",
                    flush=True
                )

    speedup, lower, upper = compare(pairs)

    assert expected_nodes is not None

    print(f"\nNodes per bench: {expected_nodes:,} (identical in every run)")

    print(f"Baseline mean:  {statistics.fmean(pair[0] for pair in pairs):,.0f} NPS")
    print(f"Candidate mean: {statistics.fmean(pair[1] for pair in pairs):,.0f} NPS")

    print(f"Speedup (paired geometric mean): {speedup:+.4f}%")
    print(f"Approx. 95% CI: [{lower:+.4f}%, {upper:+.4f}%]")
    print(f"Result: {verdict(lower, upper)}")

    print("Positive means candidate faster. This is a speed test, not an Elo test.")
    print("CI assumes independent paired log ratios; systematic bias is not covered.")

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nInterrupted; no statistical conclusion.", file=sys.stderr)
        sys.exit(130)
    except (OSError, RuntimeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
