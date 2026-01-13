#!/usr/bin/env python3
"""Plot polynomial multiplication timings from ergasia3/exercise1 MPI results."""

from __future__ import annotations

import argparse
import pathlib
from typing import Dict, List, Tuple

try:
    import matplotlib.pyplot as plt
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "matplotlib is required. Install it with 'python3 -m pip install matplotlib'."
    ) from exc

Row = Tuple[int, float, Dict[int, float], Dict[int, Dict[str, float]]]
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
DEFAULT_RESULTS = ROOT_DIR / "results" / "1.txt"
DEFAULT_OUT_DIR = ROOT_DIR / "plots"


def parse_results(path: pathlib.Path) -> List[Row]:
    rows: List[Row] = []

    current_degree: int | None = None
    sequential: float | None = None
    parallel_total: Dict[int, float] = {}
    parallel_parts: Dict[int, Dict[str, float]] = {}

    current_proc: int | None = None

    def flush() -> None:
        nonlocal current_degree, sequential, parallel_total, parallel_parts
        if current_degree is not None and sequential is not None:
            rows.append((current_degree, sequential, dict(parallel_total), dict(parallel_parts)))

    with path.open(encoding="utf-8") as fh:
        for raw in fh:
            line = raw.rstrip("\n")
            s = line.strip()

            if s.startswith("Testing degree ="):
                flush()
                current_degree = int(s.split("=")[1].strip())
                sequential = None
                parallel_total.clear()
                parallel_parts.clear()
                current_proc = None
                continue

            if s.startswith("Sequential multiplication average:"):
                sequential = float(s.split(":", 1)[1].split()[0])
                continue

            if s.startswith("Parallel multiplication with") and "processes average" in s:
                # Example: Parallel multiplication with 4 processes average: 0.123456 seconds
                parts = s.replace(":", "").split()
                try:
                    proc = int(parts[3])
                    value = float(parts[-2])
                except (IndexError, ValueError) as exc:
                    raise SystemExit(f"Failed to parse line: '{s}'") from exc
                parallel_total[proc] = value
                parallel_parts.setdefault(proc, {})
                current_proc = proc
                continue

            # Optional component timing lines (indented)
            if current_proc is not None and s.startswith("Send average:"):
                parallel_parts[current_proc]["send"] = float(s.split(":", 1)[1].split()[0])
                continue
            if current_proc is not None and s.startswith("Compute average:"):
                parallel_parts[current_proc]["compute"] = float(s.split(":", 1)[1].split()[0])
                continue
            if current_proc is not None and s.startswith("Receive average:"):
                parallel_parts[current_proc]["receive"] = float(s.split(":", 1)[1].split()[0])
                continue

    flush()

    if not rows:
        raise SystemExit(f"No summary data found in {path}")
    return rows


def plot(rows: List[Row], output_dir: pathlib.Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    for degree, seq_time, par_totals, _parts in rows:
        plt.figure(figsize=(8, 5))
        procs = sorted(par_totals)
        times = [par_totals[p] for p in procs]

        plt.plot(procs, times, marker="o", label="Parallel (MPI)")
        plt.axhline(seq_time, color="red", linestyle="--", label="Sequential avg")

        plt.title(f"Polynomial multiplication timings (MPI, degree={degree})")
        plt.xlabel("MPI processes")
        plt.ylabel("Time (s)")
        plt.grid(True, linestyle="--", alpha=0.5)
        plt.legend()
        plt.tight_layout()

        out_file = output_dir / f"exercise1_degree_{degree}.png"
        plt.savefig(out_file)
        plt.close()
        print(f"Wrote {out_file}")


def plot_summary_table(rows: List[Row], output_dir: pathlib.Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    all_procs = sorted({p for (_deg, _seq, par_totals, _parts) in rows for p in par_totals.keys()})
    col_labels = [str(p) for p in all_procs] + ["Sequential"]

    cell_text: List[List[str]] = []
    row_labels: List[str] = []
    for degree, seq_time, par_totals, _parts in rows:
        row_labels.append(f"POLY. DEGREE: {degree}")
        row = [f"{par_totals.get(p, float('nan')):.6f}" for p in all_procs]
        row.append(f"{seq_time:.6f}")
        cell_text.append(row)

    fig_w = max(8.0, 1.2 * len(col_labels))
    fig_h = 2.5 + 0.5 * len(row_labels)
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
    table.scale(1, 1.6)

    ax.set_title("Parallel multiplication (MPI) — averages (s)", pad=12)
    out_file = output_dir / "exercise1_summary_table.png"
    fig.tight_layout()
    fig.savefig(out_file, dpi=200)
    plt.close(fig)
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
        default=str(DEFAULT_OUT_DIR),
        help=f"Directory for plots (default: {DEFAULT_OUT_DIR})",
    )
    parser.add_argument(
        "--table",
        action="store_true",
        help="Also generate a summary table PNG (exercise1_summary_table.png).",
    )
    parser.add_argument(
        "--table-only",
        action="store_true",
        help="Generate only the summary table PNG (no per-degree line plots).",
    )
    args = parser.parse_args()

    rows = parse_results(pathlib.Path(args.results))
    out_dir = pathlib.Path(args.output_dir)

    if args.table_only:
        plot_summary_table(rows, out_dir)
        return

    plot(rows, out_dir)
    if args.table:
        plot_summary_table(rows, out_dir)


if __name__ == "__main__":
    main()
