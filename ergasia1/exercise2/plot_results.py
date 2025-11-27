#!/usr/bin/env python3
"""Plot synchronization benchmark averages (exercise 2)."""
from __future__ import annotations

import argparse
import pathlib
import re
from collections import defaultdict
from typing import DefaultDict, Dict, List, Tuple

try:
    import matplotlib.pyplot as plt
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "matplotlib is required. Install it with 'python -m pip install matplotlib'."
    ) from exc

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent


def _default_results_path() -> pathlib.Path:
    for name in ("2c.txt", "2.txt"):
        candidate = ROOT_DIR / "results" / name
        if candidate.exists():
            return candidate
    return ROOT_DIR / "results" / "2c.txt"


DEFAULT_RESULTS = _default_results_path()
DEFAULT_OUTPUT_DIR = ROOT_DIR / "plots"
MethodData = Dict[str, List[Tuple[int, float]]]
BenchmarkData = Dict[int, MethodData]

RUNNING_RE = re.compile(r"threads\s*=\s*(\d+)")

LineHandler = Tuple[str, str]
AVERAGE_PREFIXES: Tuple[LineHandler, ...] = (
    ("Mutex average time:", "Mutex"),
    ("RWLock average time:", "RWLock"),
    ("Atomic average time:", "Atomic"),
)
ELAPSED_PREFIXES: Tuple[LineHandler, ...] = (
    ("Elapsed time with mutex", "Mutex"),
    ("Elapsed time with rwlock", "RWLock"),
    ("Elapsed time with atomic", "Atomic"),
)


def parse_results(path: pathlib.Path) -> BenchmarkData:
    raw: DefaultDict[int, DefaultDict[str, DefaultDict[int, List[float]]]] = defaultdict(
        lambda: defaultdict(lambda: defaultdict(list))
    )
    current_iters = None
    current_threads = None

    with path.open(encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line:
                continue
            if line.startswith("Testing iterations ="):
                try:
                    current_iters = int(line.split("=")[1])
                except ValueError as exc:  # pragma: no cover
                    raise SystemExit(f"Failed to parse iterations from '{line}'") from exc
            elif line.startswith("Running:"):
                match = RUNNING_RE.search(line)
                if not match:
                    raise SystemExit(f"Failed to parse thread count from '{line}'")
                current_threads = int(match.group(1))
                continue

            matched = False
            for prefix, label in AVERAGE_PREFIXES:
                if line.startswith(prefix):
                    value = float(line.split(":")[1].split()[0])
                    _append_measure(raw, current_iters, current_threads, label, value)
                    matched = True
                    break
            if matched:
                continue

            for prefix, label in ELAPSED_PREFIXES:
                if line.startswith(prefix):
                    value = float(line.split(":")[1].split()[0])
                    _append_measure(raw, current_iters, current_threads, label, value)
                    break

    if not raw:
        raise SystemExit(f"No summary rows found in {path}")
    return _finalize(raw)


def _append_measure(
    data: DefaultDict[int, DefaultDict[str, DefaultDict[int, List[float]]]],
    iterations: int | None,
    threads: int | None,
    method: str,
    value: float,
) -> None:
    if iterations is None or threads is None:
        raise SystemExit(
            f"Encountered average line for {method} before iteration/thread context was set"
        )
    data[iterations][method][threads].append(value)


def _finalize(
    raw: DefaultDict[int, DefaultDict[str, DefaultDict[int, List[float]]]]
) -> BenchmarkData:
    finalized: BenchmarkData = {}
    for iterations, methods in raw.items():
        finalized[iterations] = {}
        for method, samples in methods.items():
            averaged: List[Tuple[int, float]] = []
            for threads, values in samples.items():
                avg = sum(values) / len(values)
                averaged.append((threads, avg))
            averaged.sort(key=lambda item: item[0])
            finalized[iterations][method] = averaged
    return finalized


def plot(data: BenchmarkData, output_dir: pathlib.Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    palette = {"Mutex": "tab:blue", "RWLock": "tab:orange", "Atomic": "tab:green"}

    for iterations, methods in sorted(data.items()):
        plt.figure(figsize=(8, 5))
        for method_name, samples in sorted(methods.items()):
            samples.sort(key=lambda item: item[0])
            threads, times = zip(*samples)
            plt.plot(threads, times, marker="o", label=method_name, color=palette.get(method_name))
        plt.title(f"Exercise 2: iterations={iterations}")
        plt.xlabel("Threads")
        plt.ylabel("Average time (s)")
        plt.grid(True, linestyle="--", alpha=0.5)
        plt.legend()
        plt.tight_layout()
        out_file = output_dir / f"exercise2_iter_{iterations}.png"
        plt.savefig(out_file)
        plt.close()
        print(f"Wrote {out_file}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "results",
        nargs="?",
        default=str(DEFAULT_RESULTS),
        help=f"Path to results file (default: {DEFAULT_RESULTS})",
    )
    parser.add_argument(
        "--output-dir",
        default=str(DEFAULT_OUTPUT_DIR),
        help=f"Directory to store PNGs (default: {DEFAULT_OUTPUT_DIR})",
    )
    args = parser.parse_args()

    results_path = pathlib.Path(args.results)
    benchmark_data = parse_results(results_path)
    plot(benchmark_data, pathlib.Path(args.output_dir))


if __name__ == "__main__":
    main()
