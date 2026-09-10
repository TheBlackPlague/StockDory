#!/usr/bin/env python3
"""Randomized paired PERFT measurements with a simultaneous regression gate."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess


POSITIONS = ("initial", "kiwipete", "position-3", "position-4", "position-4-mirrored", "position-5", "position-6")


def quantile(values, fraction):
    values = sorted(values)
    location = fraction * (len(values) - 1)
    low = int(location)
    high = min(low + 1, len(values) - 1)
    return values[low] + (values[high] - values[low]) * (location - low)


def summarize(pairs, seed, positions=POSITIONS, family_size=len(POSITIONS)):
    rng = random.Random(seed)
    reports = {}
    # Retain the full comparison family even for a focused confirmation run.
    tail = 0.05 / (2 * family_size)
    for name in positions:
        values = [pair for pair in pairs if pair["position"] == name]
        ratios = [math.log(value["candidate_seconds"] / value["baseline_seconds"]) for value in values]
        distribution = [math.exp(statistics.fmean(rng.choices(ratios, k=len(ratios)))) for _ in range(50000)]
        low, high = quantile(distribution, tail), quantile(distribution, 1 - tail)
        ratio = math.exp(statistics.fmean(ratios))
        reports[name] = {
            "pairs": len(ratios),
            "baseline_median_seconds": statistics.median(value["baseline_seconds"] for value in values),
            "candidate_median_seconds": statistics.median(value["candidate_seconds"] for value in values),
            "candidate_over_baseline_geometric_mean": ratio,
            "speedup_percent": (1 / ratio - 1) * 100,
            "simultaneous_95_percent_interval": [low, high],
            "status": "improvement" if high < 1 else "regression" if low > 1 else "inconclusive",
        }
    return reports


def measure(binary, name, raw_path, iterations):
    output = subprocess.check_output([str(binary), "--epochs", "1", "--iterations", str(iterations),
                                      "--position", name], text=True)
    raw_path.write_text(output)
    result = json.loads(output)["results"][0]
    values = result["measurements"]
    if len(values) != 1 or values[0]["iterations"] != iterations:
        raise RuntimeError("Timed PERFT iteration count differs from the requested sample size")
    # nanobench Result::add already normalizes elapsed by its iteration count.
    return values[0]["elapsed"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cpu", type=int, required=True)
    parser.add_argument("--pairs", type=int, default=31)
    parser.add_argument("--iterations", type=int, default=16)
    parser.add_argument("--position", choices=POSITIONS)
    parser.add_argument("--confidence-family-size", type=int, default=len(POSITIONS))
    parser.add_argument("--seed", type=int, default=20260910)
    args = parser.parse_args()
    if args.pairs < 21:
        parser.error("The release comparison requires at least 21 independent pairs.")
    if args.iterations < 1:
        parser.error("Each sample must contain at least one traversal.")
    selected = (args.position,) if args.position else POSITIONS
    if args.confidence_family_size < len(POSITIONS):
        parser.error("Confidence correction must retain at least the complete seven-workload family.")
    if args.cpu not in os.sched_getaffinity(0):
        parser.error("Selected CPU is outside the process affinity mask.")
    os.sched_setaffinity(0, {args.cpu})

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    raw = output / "raw"
    raw.mkdir(exist_ok=True)
    binaries = {"baseline": args.baseline.resolve(), "candidate": args.candidate.resolve()}
    manifest = {
        "started_utc": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "cpu": args.cpu,
        "cpuinfo": Path("/proc/cpuinfo").read_text() if Path("/proc/cpuinfo").exists() else None,
        "pairs_per_position": args.pairs,
        "seed": args.seed,
        "positions": selected,
        "confidence_family_size": args.confidence_family_size,
        "warmups_per_process": 2,
        "timed_invocations_per_process": args.iterations,
        "binaries": {name: {"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                     for name, path in binaries.items()},
        "build_manifests": {name: json.loads((path.parent / "build-manifest.json").read_text())
                            for name, path in binaries.items()},
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")

    rng = random.Random(args.seed)
    pairs = []
    with (output / "pairs.jsonl").open("w") as results:
        for iteration in range(args.pairs):
            positions = list(selected)
            rng.shuffle(positions)
            for name in positions:
                order = ["baseline", "candidate"]
                rng.shuffle(order)
                record = {"pair": iteration, "position": name, "order": order}
                for label in order:
                    record[f"{label}_seconds"] = measure(binaries[label], name,
                                                        raw / f"{iteration:02}-{name}-{label}.json", args.iterations)
                pairs.append(record)
                results.write(json.dumps(record) + "\n")
                results.flush()
            print(f"Completed pair {iteration + 1}/{args.pairs}", flush=True)

    # Replace the complete checkpoint once all measurements have finished.
    checkpoint = output / "pairs-complete.jsonl"
    checkpoint.write_text("".join(json.dumps(pair) + "\n" for pair in pairs))
    checkpoint.replace(output / "pairs.jsonl")

    reports = summarize(pairs, args.seed, selected, args.confidence_family_size)
    report = {
        "method": "Paired bootstrap of mean log runtime ratios; 50,000 resamples; Bonferroni simultaneous 95% intervals",
        "ratio_interpretation": "candidate runtime / baseline runtime; lower is faster",
        "confidence_family_size": args.confidence_family_size,
        "results": reports,
        "gate": "pass" if all(value["status"] == "improvement" for value in reports.values()) else "unproven",
        "finished_utc": datetime.now(timezone.utc).isoformat(),
    }
    (output / "comparison.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return 0 if report["gate"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
