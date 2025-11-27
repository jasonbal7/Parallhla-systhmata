#!/usr/bin/env python3
"""Plot timing results from 4.txt as line charts."""
from __future__ import annotations

import argparse
import pathlib
from collections import defaultdict
from typing import Dict, List, Tuple

try:
    import matplotlib.pyplot as plt
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "matplotlib is required to run this script. "
        "Install it with 'python -m pip install matplotlib'."
    ) from exc

Row = Tuple[int, int, int, int, str, str, float]
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
DEFAULT_TABLE = ROOT_DIR / "results" / "4.txt"
DEFAULT_OUT_DIR = ROOT_DIR / "plots"


def parse_table(path: pathlib.Path) -> List[Row]:
    """Parse the fixed-width table produced in 4.txt."""
    rows: List[Row] = []
    with path.open(encoding="utf-8") as fh:
        for raw_line in fh:
            line = raw_line.strip()
            if not line.startswith("|"):
                continue
            parts = [item.strip() for item in line.strip("|").split("|")]
            if not parts or parts[0] == "threads":
                continue
            threads = int(parts[0])
            accounts = int(parts[1])
            txns_per_thread = int(parts[2])
            pct = int(parts[3])
            lock = parts[4]
            variant = parts[5]
            mean_time = float(parts[6])
            rows.append((threads, accounts, txns_per_thread, pct, lock, variant, mean_time))
    if not rows:
        raise SystemExit(f"No data rows found in {path}")
    return rows


def build_series(rows: List[Row]) -> Dict[int, Dict[Tuple[str, str], List[Tuple[int, float]]]]:
    """Group rows by transactions-per-thread and then by (lock, variant)."""
    grouped: Dict[int, Dict[Tuple[str, str], List[Tuple[int, float]]]] = defaultdict(lambda: defaultdict(list))
    for _, accounts, txns, _pct, lock, variant, mean_time in rows:
        grouped[txns][(lock, variant)].append((accounts, mean_time))
    for series in grouped.values():
        for records in series.values():
            records.sort(key=lambda item: item[0])
    return grouped


def plot(series: Dict[int, Dict[Tuple[str, str], List[Tuple[int, float]]]], threads: int, pct: int, out_dir: pathlib.Path) -> None:
    """Render a PNG per transactions-per-thread bucket."""
    palette = {
        ("mutex", "coarse"): "tab:blue",
        ("mutex", "fine"): "tab:cyan",
        ("rwlock", "coarse"): "tab:red",
        ("rwlock", "fine"): "tab:orange",
    }
    out_dir.mkdir(parents=True, exist_ok=True)

    for txns_per_thread, buckets in sorted(series.items()):
        plt.figure(figsize=(8, 5))
        for key, data_points in sorted(buckets.items()):
            accounts, mean_times = zip(*data_points)
            label = f"{key[0]} ({key[1]})"
            color = palette.get(key)
            plt.plot(accounts, mean_times, marker="o", label=label, color=color)
        plt.title(
            f"Threads: {threads}  Txns/thread: {txns_per_thread}  Show %: {pct}"
        )
        plt.xlabel("Accounts")
        plt.ylabel("Mean time (s)")
        plt.grid(True, which="both", linestyle="--", alpha=0.4)
        plt.legend()
        plt.tight_layout()
        output_path = out_dir / f"exercise4_txns_{txns_per_thread}.png"
        plt.savefig(output_path)
        plt.close()
        print(f"Wrote {output_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "table",
        nargs="?",
        default=str(DEFAULT_TABLE),
        help=f"Path to the results table (default: {DEFAULT_TABLE})",
    )
    parser.add_argument(
        "--output-dir",
        default=str(DEFAULT_OUT_DIR),
        help=f"Directory to store generated PNG files (default: {DEFAULT_OUT_DIR})",
    )
    args = parser.parse_args()

    table_path = pathlib.Path(args.table)
    rows = parse_table(table_path)
    threads = rows[0][0]
    pct = rows[0][3]

    series = build_series(rows)
    plot(series, threads, pct, pathlib.Path(args.output_dir))


if __name__ == "__main__":
    main()
