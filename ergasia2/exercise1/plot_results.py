#!/usr/bin/env python3
"""Plot polynomial multiplication timings from ergasia2/exercise1 results."""
from __future__ import annotations

import argparse
import pathlib
from typing import Dict, List, Tuple

try:
    import matplotlib.pyplot as plt
except ImportError as exc:  #pragma: no cover
    raise SystemExit(
        "matplotlib is required. Install it with 'python -m pip install matplotlib'."
    ) from exc

Row = Tuple[int, float, Dict[int, float]]
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
DEFAULT_RESULTS = ROOT_DIR / "results" / "1.txt"
DEFAULT_OUT_DIR = ROOT_DIR / "plots"


def parse_results(path: pathlib.Path) -> List[Row]:
    rows: List[Row] = []
    current_degree = None
    parallel_data: Dict[int, float] = {}
    sequential = None

    with path.open(encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if line.startswith("Testing degree ="):
                if current_degree is not None and sequential is not None:
                    rows.append((current_degree, sequential, dict(parallel_data)))
                current_degree = int(line.split("=")[1])
                sequential = None
                parallel_data.clear()
            elif line.startswith("Sequential multiplication average:"):
                sequential = float(line.split(":")[1].split()[0])
            elif line.startswith("Parallel multiplication with"):
                parts = line.replace(":", "").split()
                try:
                    threads = int(parts[3])
                    value = float(parts[-2])
                except (IndexError, ValueError) as exc:
                    raise SystemExit(f"Failed to parse line: '{line}'") from exc
                parallel_data[threads] = value
    if current_degree is not None and sequential is not None:
        rows.append((current_degree, sequential, dict(parallel_data)))
    if not rows:
        raise SystemExit(f"No summary data found in {path}")
    return rows


def plot(rows: List[Row], output_dir: pathlib.Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    for degree, seq_time, par_dict in rows:
        plt.figure(figsize=(8, 5))
        threads = sorted(par_dict)
        times = [par_dict[t] for t in threads]
        plt.plot(threads, times, marker="o", label="Parallel")
        plt.axhline(seq_time, color="red", linestyle="--", label="Sequential avg")
        plt.title(f"Polynomial multiplication timings (degree={degree})")
        plt.xlabel("Threads")
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

    all_threads = sorted({t for (_, _, par_dict) in rows for t in par_dict.keys()})
    col_labels = [str(t) for t in all_threads] + ["Sequential"]

    cell_text: List[List[str]] = []
    row_labels: List[str] = []
    for degree, seq_time, par_dict in rows:
        row_labels.append(f"POLY. DEGREE: {degree}")
        row = [f"{par_dict.get(t, float('nan')):.4f}" for t in all_threads]
        row.append(f"{seq_time:.4f}")
        cell_text.append(row)

    # Wider figure when many thread columns exist.
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

    ax.set_title("Parallel multiplication (OpenMP) — averages (s)", pad=12)
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
