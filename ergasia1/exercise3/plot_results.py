#!/usr/bin/env python3
"""Plot array statistics benchmark timings (exercise 3)."""
from __future__ import annotations

import argparse
import pathlib
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
METHOD_ORDER = ("Initialization", "Serial", "Parallel")


def _default_results_path() -> pathlib.Path:
    for name in ("3c.txt", "3runs-4.txt", "3.txt"):
        candidate = ROOT_DIR / "results" / name
        if candidate.exists():
            return candidate
    return ROOT_DIR / "results" / "3c.txt"


DEFAULT_RESULTS = _default_results_path()
DEFAULT_OUTPUT_DIR = ROOT_DIR / "plots"
Series = Dict[str, List[Tuple[int, float]]]


def parse_results(path: pathlib.Path) -> Series:
    raw: DefaultDict[int, DefaultDict[str, List[float]]] = defaultdict(
        lambda: defaultdict(list)
    )
    averaged_keys: set[Tuple[int, str]] = set()
    current_size = None

    with path.open(encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line:
                continue
            if line.startswith("Testing size ="):
                try:
                    current_size = int(line.split("=")[1])
                except ValueError as exc:  # pragma: no cover
                    raise SystemExit(f"Failed to parse size from '{line}'") from exc
                continue

            if _try_record_average(line, current_size, raw, averaged_keys):
                continue
            _try_record_sample(line, current_size, raw, averaged_keys)

    if not raw:
        raise SystemExit(f"No timing information found in {path}")

    return _finalize(raw)


def _try_record_average(
    line: str,
    size: int | None,
    raw: DefaultDict[int, DefaultDict[str, List[float]]],
    averaged_keys: set[Tuple[int, str]],
) -> bool:
    mapping = {
        "Initialization average time:": "Initialization",
        "Serial average time:": "Serial",
        "Parallel average time:": "Parallel",
    }
    for prefix, label in mapping.items():
        if line.startswith(prefix):
            if size is None:
                raise SystemExit(f"Average line '{line}' appeared before any size header")
            value = float(line.split(":")[1].split()[0])
            averaged_keys.add((size, label))
            raw[size][label] = [value]
            return True
    return False


def _try_record_sample(
    line: str,
    size: int | None,
    raw: DefaultDict[int, DefaultDict[str, List[float]]],
    averaged_keys: set[Tuple[int, str]],
) -> None:
    mapping = {
        "Arrays creation time:": "Initialization",
        "Serial computation time:": "Serial",
        "Parallel computation time:": "Parallel",
    }
    for prefix, label in mapping.items():
        if line.startswith(prefix):
            if size is None:
                raise SystemExit(f"Timing line '{line}' appeared before any size header")
            if (size, label) in averaged_keys:
                return
            value = float(line.split(":")[1].split()[0])
            raw[size][label].append(value)
            return


def _finalize(raw: DefaultDict[int, DefaultDict[str, List[float]]]) -> Series:
    series: Series = {method: [] for method in METHOD_ORDER}
    for size, methods in sorted(raw.items()):
        for method, values in methods.items():
            if not values:
                continue
            avg = sum(values) / len(values)
            series.setdefault(method, []).append((size, avg))
    # Remove empty placeholders to keep plotting tidy
    return {method: points for method, points in series.items() if points}


def plot(series: Series, output_dir: pathlib.Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    plt.figure(figsize=(8, 5))
    palette = {
        "Initialization": "tab:purple",
        "Serial": "tab:orange",
        "Parallel": "tab:blue",
    }

    for method in METHOD_ORDER:
        points = series.get(method)
        if not points:
            continue
        points.sort(key=lambda item: item[0])
        sizes, times = zip(*points)
        plt.plot(sizes, times, marker="o", label=method, color=palette.get(method))

    plt.xlabel("Array size")
    plt.ylabel("Average time (s)")
    plt.title("Exercise 3: array statistics timings")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    out_file = output_dir / "exercise3_timings.png"
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
        help=f"Directory for rendered PNGs (default: {DEFAULT_OUTPUT_DIR})",
    )
    args = parser.parse_args()

    results_path = pathlib.Path(args.results)
    data_series = parse_results(results_path)
    plot(data_series, pathlib.Path(args.output_dir))


if __name__ == "__main__":
    main()
