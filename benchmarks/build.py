#!/usr/bin/env python3
"""Build the harnesses with a configured StockDory Ninja build's exact flags."""

import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine-build", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--nanobench", type=Path, required=True)
    parser.add_argument("--board-type", default="StockDory::Board")
    parser.add_argument("--baseline", action="store_true")
    args = parser.parse_args()

    build = args.engine_build.resolve()
    source = args.source.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    harness = Path(__file__).resolve().parent.parent
    cache = dict(line.split("=", 1) for line in (build / "CMakeCache.txt").read_text().splitlines()
                 if "=" in line and not line.startswith(("#", "//")))
    ninja = next(value for key, value in cache.items() if key.startswith("CMAKE_MAKE_PROGRAM:"))
    commands = subprocess.check_output([ninja, "-C", str(build), "-t", "commands", "StockDory"], text=True)
    compile_line = next(line for line in commands.splitlines() if " -c " in line and line.endswith("/src/main.cpp"))
    compile_args = shlex.split(compile_line)
    compiler = compile_args[0]
    compile_flags = []
    index = 1
    while index < len(compile_args):
        token = compile_args[index]
        if token in ("-MT", "-MF", "-o", "-c"):
            index += 2
        elif token == "-MD":
            index += 1
        else:
            compile_flags.append(token)
            index += 1

    link_line = next(line for line in commands.splitlines()
                     if "CMakeFiles/StockDory.dir/src/main.cpp.o -o StockDory " in line)
    link_args = shlex.split(link_line)
    link_args = link_args[link_args.index(compiler) + 1:]
    link_flags = []
    index = 0
    while index < len(link_args):
        token = link_args[index]
        if token == "-o" or (token == "-Xlinker" and link_args[index + 1].startswith("--dependency-file=")):
            index += 2
        elif token in ("&&", ":") or token.endswith("/src/main.cpp.o"):
            index += 1
        else:
            link_flags.append(token)
            index += 1

    if any("-fprofile" in flag for flag in compile_flags + link_flags):
        raise SystemExit("Refusing a PGO/instrumented engine build: configure BUILD_PGO=OFF.")
    if "-O3" not in compile_flags:
        raise SystemExit("Use the project's Release build for the performance gate.")

    nanobench = args.nanobench.resolve()
    extra = [f"-I{source / 'src'}", f"-I{nanobench.parent}", f"-DSTOCKDORY_TEST_BOARD_TYPE={args.board_type}"]
    if args.baseline:
        extra.append("-DSTOCKDORY_BASELINE")

    built = []
    targets = [("tests/BackendPerft.cpp", "BackendPerft"),
               ("benchmarks/BackendPerft.cpp", "BackendPerftBenchmark")]
    if not args.baseline:
        targets.append(("tests/MoveFlags.cpp", "MoveFlags"))
        targets.append(("tests/BackendState.cpp", "BackendState"))
    for relative, name in targets:
        object_file = output / f"{name}.o"
        executable = output / name
        compile_command = [compiler, *compile_flags, *extra, "-c", str(harness / relative), "-o", str(object_file)]
        link_command = [compiler, *link_flags, str(object_file), "-o", str(executable)]
        subprocess.run(compile_command, cwd=build, check=True)
        subprocess.run(link_command, cwd=build, check=True)
        built.append({"executable": str(executable), "sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
                      "compile": compile_command, "link": link_command})

    source_files = subprocess.check_output(
        ["git", "-C", str(source), "ls-files", "--cached", "--others", "--exclude-standard", "--", "src"], text=True
    ).splitlines()
    source_digest = hashlib.sha256()
    source_hashes = {}
    for relative in sorted(set(source_files)):
        path = source / relative
        if path.is_file():
            content = path.read_bytes()
            source_hashes[relative] = hashlib.sha256(content).hexdigest()
            source_digest.update(relative.encode() + b"\0" + content)

    manifest = {
        "source": str(source),
        "commit": subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip(),
        "source_tree_sha256": source_digest.hexdigest(),
        "source_files_sha256": source_hashes,
        "compiler": subprocess.check_output([compiler, "--version"], text=True).strip(),
        "board_type": args.board_type,
        "baseline_depth_zero_compatibility": args.baseline,
        "nanobench_sha256": hashlib.sha256(nanobench.read_bytes()).hexdigest(),
        "builds": built,
    }
    (output / "build-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(output)


if __name__ == "__main__":
    main()
