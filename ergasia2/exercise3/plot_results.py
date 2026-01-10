#!/usr/bin/env python3
"""Plot Exercise 3 mergesort timings.

Parses the table produced by ergasia2/exercise3/2.3.sh and generates:
- One PNG per array size with time+speedup curves
- Optional table-style PNG summary

Expected input rows:
  <ArraySize> <Threads> <AvgTimeSeconds>
"""

from __future__ import annotations

import argparse
import math
import pathlib
from dataclasses import dataclass
from typing import Dict, List, Tuple

try:
    import matplotlib.pyplot as plt
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "matplotlib is required. Install it with 'python -m pip install matplotlib'."
    ) from exc

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
DEFAULT_RESULTS = ROOT_DIR / "results" / "3.txt"
DEFAULT_OUT_DIR = ROOT_DIR / "plots"


@dataclass(frozen=True)
class Row:
    n: int
    threads: int
    time_s: float


def parse_results(path: pathlib.Path) -> List[Row]:
    rows: List[Row] = []
    with path.open(encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line:
                continue
            if line.startswith("Array") or line.startswith("-"):
                continue

            parts = line.split()
            if len(parts) < 3:
                continue

            try:
                n = int(parts[0])
                threads = int(parts[1])
                time_s = float(parts[2])
            except ValueError:
                continue

            rows.append(Row(n=n, threads=threads, time_s=time_s))

    if not rows:
        raise SystemExit(f"No data rows parsed from {path}")

    return rows


def _group(rows: List[Row]) -> Dict[int, Dict[int, float]]:
    by_n: Dict[int, Dict[int, float]] = {}
    for r in rows:
        by_n.setdefault(r.n, {})[r.threads] = r.time_s
    return by_n


def plot_per_size(by_n: Dict[int, Dict[int, float]], out_dir: pathlib.Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    for n in sorted(by_n.keys()):
        series = by_n[n]
        threads = sorted(series.keys())
        times = [series[t] for t in threads]

        # Speedup relative to 1-thread time if present
        t1 = series.get(1)
        speedups = [(t1 / series[t]) if (t1 is not None and series[t] > 0) else math.nan for t in threads]

        fig, axes = plt.subplots(1, 2, figsize=(12, 4.5))

        ax_t = axes[0]
        ax_t.plot(threads, times, marker="o")
        ax_t.set_title(f"Avg time vs threads (N={n})")
        ax_t.set_xlabel("Threads")
        ax_t.set_ylabel("Time (s)")
        ax_t.grid(True, linestyle="--", alpha=0.5)

        ax_s = axes[1]
        ax_s.plot(threads, speedups, marker="o")
        ax_s.set_title(f"Speedup vs threads (N={n})")
        ax_s.set_xlabel("Threads")
        ax_s.set_ylabel("Speedup (T1 / Tp)")
        ax_s.grid(True, linestyle="--", alpha=0.5)

        fig.suptitle("Exercise 3 — mergesort")
        fig.tight_layout()
        out_path = out_dir / f"exercise3_N{n}.png"
        fig.savefig(out_path, dpi=200)
        plt.close(fig)
        print(f"Wrote {out_path}")


def plot_summary_table(by_n: Dict[int, Dict[int, float]], out_dir: pathlib.Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    all_threads = sorted({t for series in by_n.values() for t in series.keys()})
    has_serial = 1 in all_threads
    parallel_threads = [t for t in all_threads if t != 1]
    if has_serial:
        col_labels = ["Serial"] + [str(t) for t in parallel_threads]
    else:
        col_labels = [str(t) for t in all_threads]

    row_labels: List[str] = []
    cell_text: List[List[str]] = []

    for n in sorted(by_n.keys()):
        row_labels.append(f"N={n}")
        row: List[str] = []
        if has_serial:
            v = by_n[n].get(1)
            row.append(f"{v:.3f}" if v is not None else "-")
            for t in parallel_threads:
                v = by_n[n].get(t)
                row.append(f"{v:.3f}" if v is not None else "-")
        else:
            for t in all_threads:
                v = by_n[n].get(t)
                row.append(f"{v:.3f}" if v is not None else "-")
        cell_text.append(row)

    fig_w = max(8.0, 1.1 * len(col_labels) + 3.0)
    fig_h = max(3.0, 0.6 * len(row_labels) + 1.5)

    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    ax.axis("off")
    table = ax.table(
        cellText=cell_text,
        rowLabels=row_labels,
        colLabels=col_labels,
        cellLoc="center",
        loc="center",
    )
    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1, 1.5)

    title = "Exercise 3 — average time (seconds)"
    if has_serial:
        title += " (Serial = 1 thread)"
    ax.set_title(title, pad=12)
    out_path = out_dir / "exercise3_summary_table.png"
    fig.tight_layout()
    fig.savefig(out_path, dpi=200)
    plt.close(fig)
    print(f"Wrote {out_path}")


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
        default=str(DEFAULT_OUT_DIR),
        help=f"Directory for plots (default: {DEFAULT_OUT_DIR})",
    )
    parser.add_argument(
        "--table",
        action="store_true",
        help="Also generate a summary table PNG (exercise3_summary_table.png).",
    )
    parser.add_argument(
        "--table-only",
        action="store_true",
        help="Generate only the summary table PNG (no graphs).",
    )
    args = parser.parse_args()

    rows = parse_results(pathlib.Path(args.results))
    by_n = _group(rows)
    out_dir = pathlib.Path(args.output_dir)

    if args.table_only:
        plot_summary_table(by_n, out_dir)
        return

    plot_per_size(by_n, out_dir)
    if args.table:
        plot_summary_table(by_n, out_dir)


if __name__ == "__main__":
    main()
