#!/usr/bin/env python3
"""Plot barrier benchmark averages (exercise 5)."""
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
    for name in ("5runs-4.txt", "5.txt"):
        candidate = ROOT_DIR / "results" / name
        if candidate.exists():
            return candidate
    return ROOT_DIR / "results" / "5runs-4.txt"


DEFAULT_RESULTS = _default_results_path()
DEFAULT_OUTPUT_DIR = ROOT_DIR / "plots"
Series = Dict[int, Dict[str, List[Tuple[int, float]]]]

ENTRY_RE = re.compile(
    r"^(?P<label>.+?\([^)]*\))\s+(?P<threads>\d+)\s+(?P<iterations>\d+)\s+(?P<avg>\d+(?:\.\d+)?)$"
)


def parse_results(path: pathlib.Path) -> Series:
    data: DefaultDict[int, DefaultDict[str, List[Tuple[int, float]]]] = defaultdict(
        lambda: defaultdict(list)
    )

    with path.open(encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            match = ENTRY_RE.match(line)
            if not match:
                continue
            label = match.group("label").strip()
            threads = int(match.group("threads"))
            iterations = int(match.group("iterations"))
            avg_time = float(match.group("avg"))
            data[iterations][label].append((threads, avg_time))

    if not data:
        raise SystemExit(f"No summary rows found in {path}")
    return data


def plot(series: Series, output_dir: pathlib.Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    palette = {
        "pthread_barrier (5.1)": "tab:blue",
        "mutex_cond_barrier (5.2)": "tab:orange",
        "sense_reversal_barrier (5.3)": "tab:green",
    }

    for iterations, methods in sorted(series.items()):
        plt.figure(figsize=(8, 5))
        for label, samples in sorted(methods.items()):
            samples.sort(key=lambda item: item[0])
            threads, times = zip(*samples)
            display_label = label.replace("_", " ")
            plt.plot(
                threads,
                times,
                marker="o",
                label=display_label,
                color=palette.get(label, None),
            )
        plt.xlabel("Threads")
        plt.ylabel("Average time (s)")
        plt.title(f"Exercise 5: iterations={iterations}")
        plt.yscale("log")
        plt.grid(True, linestyle="--", alpha=0.5)
        plt.legend()
        plt.tight_layout()
        out_file = output_dir / f"exercise5_iter_{iterations}.png"
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
    series = parse_results(results_path)
    plot(series, pathlib.Path(args.output_dir))


if __name__ == "__main__":
    main()
