#!/usr/bin/env python3
"""Compare ergasia1/2/3 exercise1 timings on a single plot."""
from __future__ import annotations

import argparse
import pathlib
import re
from typing import Dict, List, Tuple

try:
    import matplotlib.pyplot as plt
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "matplotlib is required. Install it with 'python -m pip install matplotlib'."
    ) from exc

Series = Dict[int, Dict[str, object]]
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
DEFAULT_E1 = ROOT_DIR / "ergasia1" / "results" / "1.txt"
DEFAULT_E2 = ROOT_DIR / "ergasia2" / "results" / "1.txt"
DEFAULT_E3 = ROOT_DIR / "ergasia3" / "results" / "1.txt"
DEFAULT_OUT_DIR = SCRIPT_DIR / "Timing Comparison Plots"

SEQ_RE = re.compile(r"Sequential multiplication average:\s*([0-9.]+)")
PAR_RE = re.compile(
    r"Parallel multiplication with\s+(\d+)\s+(threads|processes)\s+average:\s*([0-9.]+)"
)


def parse_results(path: pathlib.Path) -> Series:
    data: Series = {}
    current_degree: int | None = None

    with path.open(encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if line.startswith("Testing degree ="):
                current_degree = int(line.split("=")[1])
                data[current_degree] = {"sequential": None, "parallel": {}}
                continue

            if current_degree is None:
                continue

            seq_match = SEQ_RE.match(line)
            if seq_match:
                data[current_degree]["sequential"] = float(seq_match.group(1))
                continue

            par_match = PAR_RE.match(line)
            if par_match:
                workers = int(par_match.group(1))
                value = float(par_match.group(3))
                data[current_degree]["parallel"][workers] = value

    if not data:
        raise SystemExit(f"No summary data found in {path}")
    return data


def plot(
    series: List[Tuple[str, Series]],
    output_dir: pathlib.Path,
) -> pathlib.Path:
    output_dir.mkdir(parents=True, exist_ok=True)

    degrees = sorted({deg for _label, data in series for deg in data.keys()})
    if not degrees:
        raise SystemExit("No common degrees found to plot.")

    cols = 2
    rows = (len(degrees) + cols - 1) // cols
    fig_w = 12
    fig_h = max(4.0, 3.6 * rows)
    fig, axes = plt.subplots(rows, cols, figsize=(fig_w, fig_h), sharex=False)
    if rows == 1 and cols == 1:
        axes = [[axes]]
    elif rows == 1:
        axes = [axes]

    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    label_colors = {
        label: colors[idx % len(colors)] for idx, (label, _data) in enumerate(series)
    }

    flat_axes = [ax for row in axes for ax in row]
    for ax, degree in zip(flat_axes, degrees):
        for label, data in series:
            if degree not in data:
                continue
            color = label_colors[label]
            parallel: Dict[int, float] = data[degree]["parallel"]  # type: ignore[assignment]
            if parallel:
                workers = sorted(parallel)
                times = [parallel[w] for w in workers]
                ax.plot(
                    workers,
                    times,
                    marker="o",
                    label=label,
                    color=color,
                )
            seq_time = data[degree]["sequential"]
            if isinstance(seq_time, (int, float)):
                ax.axhline(
                    float(seq_time),
                    color=color,
                    linestyle="--",
                    alpha=0.7,
                    label=f"{label} - Serial Execution",
                )

        ax.set_title(f"MPI vs OMP vs Pthreads - P. Degree: {degree}")
        ax.set_xlabel("Number of Threads")
        ax.set_ylabel("Execution Time (seconds)")
        ax.grid(True, linestyle="--", alpha=0.4)
        ax.legend(fontsize=8)

    for ax in flat_axes[len(degrees) :]:
        ax.axis("off")

    fig.tight_layout()
    out_file = output_dir / "compare_timings.png"
    fig.savefig(out_file, dpi=200)
    plt.close(fig)
    return out_file


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--ergasia1",
        default=str(DEFAULT_E1),
        help=f"Path to ergasia1 results (default: {DEFAULT_E1})",
    )
    parser.add_argument(
        "--ergasia2",
        default=str(DEFAULT_E2),
        help=f"Path to ergasia2 results (default: {DEFAULT_E2})",
    )
    parser.add_argument(
        "--ergasia3",
        default=str(DEFAULT_E3),
        help=f"Path to ergasia3 results (default: {DEFAULT_E3})",
    )
    parser.add_argument(
        "--output-dir",
        default=str(DEFAULT_OUT_DIR),
        help=f"Directory for plots (default: {DEFAULT_OUT_DIR})",
    )
    args = parser.parse_args()

    series = [
        ("OMP", parse_results(pathlib.Path(args.ergasia1))),
        ("Pthreads", parse_results(pathlib.Path(args.ergasia2))),
        ("MPI", parse_results(pathlib.Path(args.ergasia3))),
    ]

    out_file = plot(series, pathlib.Path(args.output_dir))
    print(f"Wrote {out_file}")


if __name__ == "__main__":
    main()