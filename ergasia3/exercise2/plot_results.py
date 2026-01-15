#!/usr/bin/env python3
"""Plot MPI CSR/Dense benchmark comparison (Calc vs Calc, Total vs Total)."""

from __future__ import annotations
import argparse
import pathlib
import re
from dataclasses import dataclass
from typing import Dict, List, Tuple
import matplotlib.pyplot as plt

@dataclass(frozen=True)
class CaseKey:
    n: int
    sparsity: float
    iterations: int

@dataclass
class CaseData:
    calc_csr: Dict[int, float]
    calc_dense: Dict[int, float]
    total_csr: Dict[int, float]
    total_dense: Dict[int, float]

# Regex matching the Bash script output
CASE_RE = re.compile(r"^Testing N = (\d+), Sparsity = ([0-9.]+), Iterations = (\d+)$")
PROCESS_RE = re.compile(r"^MPI Processes = (\d+)$")

# [FIX] Άλλαξα το \s+:\s+ σε \s*:\s+ για να πιάνει και περιπτώσεις χωρίς κενό πριν το :
VALUE_RE = re.compile(r"^\((.+?)\)\s+(.+?)\s*:\s+([0-9.]+)\s+sec$")

# Map log labels to internal data fields
LABEL_MAP = {
    "Avg CSR Calculation Time": "calc_csr",
    "Avg Dense Calculation Time": "calc_dense",
    "Avg TOTAL CSR Time": "total_csr",
    "Avg TOTAL DENSE Time": "total_dense",
}

def parse_results(path: pathlib.Path) -> Dict[CaseKey, CaseData]:
    cases = {}
    cur_case = None
    cur_procs = None

    with path.open(encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line: continue
            
            m = CASE_RE.match(line)
            if m:
                cur_case = CaseKey(int(m.group(1)), float(m.group(2)), int(m.group(3)))
                if cur_case not in cases:
                    cases[cur_case] = CaseData({}, {}, {}, {})
                continue

            p = PROCESS_RE.match(line)
            if p:
                cur_procs = int(p.group(1))
                continue

            v = VALUE_RE.match(line)
            if v and cur_case and cur_procs:
                label = v.group(2).strip()
                val = float(v.group(3))
                field = LABEL_MAP.get(label)
                if field:
                    getattr(cases[cur_case], field)[cur_procs] = val
    return cases

def _sorted_xy(d):
    xs = sorted(d.keys())
    return xs, [d[x] for x in xs]

def plot_case(n, it, grouped, out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    fig, axes = plt.subplots(1, 2, figsize=(18, 7))
    
    ax_calc = axes[0]
    ax_total = axes[1]
    
    sparsities = sorted(grouped.keys())
    colors = ["b", "g", "r", "c", "m"]
    markers = ["o", "s", "^", "D"]

    for i, sp in enumerate(sparsities):
        if sp not in grouped or not grouped[sp]: continue
        key, data = grouped[sp][0]
        
        c = colors[i % len(colors)]
        m = markers[i % len(markers)]

        # --- Plot 1: Calculation Time ---
        if data.calc_csr and data.calc_dense:
            x, y_csr = _sorted_xy(data.calc_csr)
            x_d, y_dense = _sorted_xy(data.calc_dense)
            ax_calc.plot(x, y_csr, marker=m, color=c, label=f"CSR (Sp={sp})")
            ax_calc.plot(x, y_dense, marker=m, color=c, linestyle='--', alpha=0.6, label=f"Dense (Sp={sp})")

        # --- Plot 2: Total Time ---
        if data.total_csr and data.total_dense:
            x, y_csr = _sorted_xy(data.total_csr)
            x_d, y_dense = _sorted_xy(data.total_dense)
            ax_total.plot(x, y_csr, marker=m, color=c, label=f"CSR (Sp={sp})")
            ax_total.plot(x, y_dense, marker=m, color=c, linestyle='--', alpha=0.6, label=f"Dense (Sp={sp})")

    ax_calc.set_title(f"Pure Calculation Time (N={n}, Iter={it})\nSolid = CSR | Dashed = Dense")
    ax_calc.set_xlabel("MPI Processes")
    ax_calc.set_ylabel("Time (s)")
    ax_calc.legend()
    ax_calc.grid(True, linestyle='--', alpha=0.5)
    
    ax_total.set_title(f"Total Execution Time (N={n}, Iter={it})\nSolid = CSR | Dashed = Dense")
    ax_total.set_xlabel("MPI Processes")
    ax_total.set_ylabel("Time (s)")
    ax_total.legend()
    ax_total.grid(True, linestyle='--', alpha=0.5)

    if data.calc_csr:
        all_procs = sorted(data.calc_csr.keys())
        ax_calc.set_xticks(all_procs)
        ax_total.set_xticks(all_procs)

    out_path = out_dir / f"Compare_N{n}_It{it}.png"
    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)
    print(f"Generated: {out_path}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("results")
    parser.add_argument("--output-dir")
    args = parser.parse_args()
    
    cases = parse_results(pathlib.Path(args.results))
    
    by_size = {}
    for k, d in cases.items():
        by_size.setdefault(k.n, {}).setdefault(k.iterations, {}).setdefault(k.sparsity, []).append((k, d))

    for n in by_size:
        for it in by_size[n]:
            plot_case(n, it, by_size[n][it], pathlib.Path(args.output_dir))

if __name__ == "__main__":
    main()