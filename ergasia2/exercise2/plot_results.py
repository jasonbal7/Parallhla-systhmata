#!/usr/bin/env python3
"""Plot CSR/Dense benchmark averages for ergasia2/exercise2.

Input file format is produced by exercise2/2.sh.
Outputs PNGs into --output-dir.
"""

from __future__ import annotations

import argparse
import pathlib
import re
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
DEFAULT_RESULTS = ROOT_DIR / "results" / "2.txt"
DEFAULT_OUT_DIR = ROOT_DIR / "plots"


@dataclass(frozen=True)
class CaseKey:
    m: int
    sparsity: float
    iterations: int


@dataclass
class CaseData:
    # thread -> value
    serial_conv: Dict[int, float]
    parallel_conv: Dict[int, float]
    csr_serial: Dict[int, float]
    csr_parallel: Dict[int, float]
    dense_serial: Dict[int, float]
    dense_parallel: Dict[int, float]


CASE_RE = re.compile(r"^Testing m = (\d+) sparsity = ([0-9.]+) iterations = (\d+)$")
THREAD_RE = re.compile(r"^Threads = (\d+)$")
VALUE_RE = re.compile(r"^(.+?)\s*average:\s*([0-9.]+)\s*seconds$")


LABEL_MAP = {
    "Serial CSR conversion": "serial_conv",
    "Parallel CSR conversion": "parallel_conv",
    "CSR serial multiplication": "csr_serial",
    "CSR parallel multiplication": "csr_parallel",
    "Dense serial multiplication": "dense_serial",
    "Dense parallel multiplication": "dense_parallel",
}


def parse_results(path: pathlib.Path) -> Dict[CaseKey, CaseData]:
    cases: Dict[CaseKey, CaseData] = {}
    cur_case: CaseKey | None = None
    cur_threads: int | None = None

    def ensure_case(key: CaseKey) -> CaseData:
        if key not in cases:
            cases[key] = CaseData(
                serial_conv={},
                parallel_conv={},
                csr_serial={},
                csr_parallel={},
                dense_serial={},
                dense_parallel={},
            )
        return cases[key]

    with path.open(encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line:
                continue

            m = CASE_RE.match(line)
            if m:
                cur_case = CaseKey(
                    m=int(m.group(1)),
                    sparsity=float(m.group(2)),
                    iterations=int(m.group(3)),
                )
                cur_threads = None
                ensure_case(cur_case)
                continue

            t = THREAD_RE.match(line)
            if t:
                cur_threads = int(t.group(1))
                continue

            v = VALUE_RE.match(line)
            if v and cur_case is not None and cur_threads is not None:
                label = v.group(1).strip()
                value = float(v.group(2))
                field = LABEL_MAP.get(label)
                if field is None:
                    continue
                getattr(ensure_case(cur_case), field)[cur_threads] = value

    if not cases:
        raise SystemExit(f"No cases parsed from {path}")
    return cases


def _sorted_xy(d: Dict[int, float]) -> Tuple[List[int], List[float]]:
    xs = sorted(d)
    ys = [d[x] for x in xs]
    return xs, ys


def plot_by_size(
    m: int,
    iterations: int,
    grouped: Dict[float, List[Tuple[CaseKey, CaseData]]],
    out_dir: pathlib.Path,
    *,
    multi_iterations: bool,
) -> None:
    """Create a single figure for a given size `m` and iteration-count.

    The figure contains two subplots:
      - Conversion times vs Threads, lines per sparsity
      - Multiplication times vs Threads, lines per sparsity (CSR & Dense)
    """
    out_dir.mkdir(parents=True, exist_ok=True)

    fig, axes = plt.subplots(1, 2, figsize=(14, 5), sharex=False)

    conv_ax = axes[0]
    mult_ax = axes[1]

    # Use a stable sorted list of sparsity values
    sparsities = sorted(grouped.keys())

    # Distinguish line styles across sparsities
    markers = ["o", "s", "^", "D", "v", "*", "x"]
    linestyles = ["-", "--", "-.", ":"]

    # Plot Conversion: parallel + optional serial baselines per sparsity
    for i, s in enumerate(sparsities):
        cases_for_s = grouped[s]
        if not cases_for_s:
            continue
        # Pick first case data for this sparsity
        key, data = cases_for_s[0]

        if data.parallel_conv:
            x, y = _sorted_xy(data.parallel_conv)
            conv_ax.plot(
                x,
                y,
                marker=markers[i % len(markers)],
                linestyle=linestyles[0],
                label=f"Conversion parallel (s={s})",
            )
        if data.serial_conv:
            any_val = next(iter(data.serial_conv.values()))
            conv_ax.axhline(
                any_val,
                linestyle=linestyles[1],
                alpha=0.6,
                label=f"Conversion serial (s={s})",
            )

    conv_ax.set_title(f"Exercise 2 Conversion — m={m}, iterations={iterations}")
    conv_ax.set_xlabel("Threads")
    conv_ax.set_ylabel("Average time (s)")
    conv_ax.grid(True, linestyle="--", alpha=0.5)
    conv_ax.legend(fontsize=8)

    # Plot Multiplication: CSR & Dense parallel + optional serial baselines per sparsity
    for i, s in enumerate(sparsities):
        cases_for_s = grouped[s]
        if not cases_for_s:
            continue
        key, data = cases_for_s[0]

        if data.csr_parallel:
            x, y = _sorted_xy(data.csr_parallel)
            mult_ax.plot(
                x,
                y,
                marker=markers[i % len(markers)],
                linestyle=linestyles[0],
                label=f"CSR parallel (s={s})",
            )
        if data.dense_parallel:
            x, y = _sorted_xy(data.dense_parallel)
            mult_ax.plot(
                x,
                y,
                marker=markers[(i + 1) % len(markers)],
                linestyle=linestyles[2],
                label=f"Dense parallel (s={s})",
            )
        if data.csr_serial:
            any_val = next(iter(data.csr_serial.values()))
            mult_ax.axhline(
                any_val,
                linestyle=linestyles[1],
                alpha=0.6,
                label=f"CSR serial (s={s})",
            )
        if data.dense_serial:
            any_val = next(iter(data.dense_serial.values()))
            mult_ax.axhline(
                any_val,
                linestyle=linestyles[3],
                alpha=0.6,
                label=f"Dense serial (s={s})",
            )

    mult_ax.set_title(f"Exercise 2 Multiplication — m={m}, iterations={iterations}")
    mult_ax.set_xlabel("Threads")
    mult_ax.set_ylabel("Average time (s)")
    mult_ax.grid(True, linestyle="--", alpha=0.5)
    mult_ax.legend(fontsize=8)

    fig.suptitle(f"Results grouped by size m={m} (iterations={iterations})")
    fig.tight_layout()
    if multi_iterations:
        out_path = out_dir / f"exercise2_m{m}_it{iterations}.png"
    else:
        out_path = out_dir / f"exercise2_m{m}.png"
    fig.savefig(out_path)
    plt.close(fig)
    print(f"Wrote {out_path}")


def plot_summary_table_by_size(
    m: int,
    iterations: int,
    grouped: Dict[float, List[Tuple[CaseKey, CaseData]]],
    out_dir: pathlib.Path,
    *,
    multi_iterations: bool,
) -> None:
    """Create a table-style summary PNG for a given size `m` and iteration-count."""
    out_dir.mkdir(parents=True, exist_ok=True)

    sparsities = sorted(grouped.keys())
    # Collect union of thread counts across all datasets.
    all_threads = sorted(
        {
            t
            for s in sparsities
            for (_, data) in (grouped[s][:1] if grouped[s] else [])
            for d in (
                data.parallel_conv,
                data.csr_parallel,
                data.dense_parallel,
            )
            for t in d.keys()
        }
    )
    if not all_threads:
        return

    col_labels = [str(t) for t in all_threads] + ["Serial"]

    row_labels: List[str] = []
    cell_text: List[List[str]] = []

    def fmt(x: float) -> str:
        return f"{x:.6f}" if x == x else "-"  # NaN-safe

    for s in sparsities:
        cases_for_s = grouped[s]
        if not cases_for_s:
            continue
        key, data = cases_for_s[0]

        # Conversion parallel row
        row_labels.append(f"Conversion parallel (s={s})")
        row = [fmt(data.parallel_conv.get(t, float('nan'))) for t in all_threads]
        row.append("-")
        cell_text.append(row)

        # Conversion serial row
        row_labels.append(f"Conversion serial (s={s})")
        row = ["-" for _ in all_threads]
        row.append(fmt(next(iter(data.serial_conv.values()))) if data.serial_conv else "-")
        cell_text.append(row)

        # CSR parallel row
        row_labels.append(f"CSR parallel mult (s={s})")
        row = [fmt(data.csr_parallel.get(t, float('nan'))) for t in all_threads]
        row.append("-")
        cell_text.append(row)

        # CSR serial row
        row_labels.append(f"CSR serial mult (s={s})")
        row = ["-" for _ in all_threads]
        row.append(fmt(next(iter(data.csr_serial.values()))) if data.csr_serial else "-")
        cell_text.append(row)

        # Dense parallel row
        row_labels.append(f"Dense parallel mult (s={s})")
        row = [fmt(data.dense_parallel.get(t, float('nan'))) for t in all_threads]
        row.append("-")
        cell_text.append(row)

        # Dense serial row
        row_labels.append(f"Dense serial mult (s={s})")
        row = ["-" for _ in all_threads]
        row.append(fmt(next(iter(data.dense_serial.values()))) if data.dense_serial else "-")
        cell_text.append(row)

    fig_w = max(10.0, 1.1 * len(col_labels) + 6.0)
    fig_h = max(4.0, 0.35 * len(row_labels) + 2.0)
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
    table.set_fontsize(9)
    table.scale(1, 1.25)

    ax.set_title(
        f"Exercise 2 summary table — m={m}, iterations={iterations} (times in seconds)",
        pad=12,
    )
    if multi_iterations:
        out_path = out_dir / f"exercise2_m{m}_it{iterations}_summary_table.png"
    else:
        out_path = out_dir / f"exercise2_m{m}_summary_table.png"
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
        "--sizes",
        type=int,
        nargs="*",
        help="Limit plotting to these matrix sizes (e.g., --sizes 1000 5000 10000)",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        nargs="*",
        help="Limit plotting to these iteration counts (e.g., --iterations 5 10 20)",
    )
    parser.add_argument(
        "--table",
        action="store_true",
        help="Also generate a summary table PNG per size (exercise2_m*_summary_table.png).",
    )
    parser.add_argument(
        "--table-only",
        action="store_true",
        help="Generate only the summary table PNGs (no line plots).",
    )
    args = parser.parse_args()

    results_path = pathlib.Path(args.results)
    out_dir = pathlib.Path(args.output_dir)

    cases = parse_results(results_path)

    # Group cases by size, then iterations, then sparsity.
    by_size: Dict[int, Dict[int, Dict[float, List[Tuple[CaseKey, CaseData]]]]] = {}
    for key, data in cases.items():
        by_size.setdefault(key.m, {}).setdefault(key.iterations, {}).setdefault(key.sparsity, []).append((key, data))

    # Create one figure per (possibly filtered) size
    sizes = sorted(by_size)
    if args.sizes:
        sizes = [m for m in sizes if m in set(args.sizes)]
        if not sizes:
            raise SystemExit("No matching sizes found in results for the provided --sizes filter")

    for m in sizes:
        iterations_available = sorted(by_size[m].keys())
        if args.iterations:
            wanted = set(args.iterations)
            iterations_available = [it for it in iterations_available if it in wanted]
            if not iterations_available:
                continue

        multi = (len(iterations_available) > 1) or bool(args.iterations)
        for it in iterations_available:
            if not args.table_only:
                plot_by_size(m, it, by_size[m][it], out_dir, multi_iterations=multi)
            if args.table or args.table_only:
                plot_summary_table_by_size(m, it, by_size[m][it], out_dir, multi_iterations=multi)


if __name__ == "__main__":
    main()
