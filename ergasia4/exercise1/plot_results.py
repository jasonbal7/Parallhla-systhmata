#!/usr/bin/env python3

import argparse
import pathlib
import sys
from typing import List, Tuple, Optional

try:
    import matplotlib.pyplot as plt
except ImportError:
    print("Error: Matplotlib is required. Install it with 'pip install matplotlib'")
    sys.exit(1)

Row = Tuple[int, float, float, Optional[float]]

def parse_results(path: pathlib.Path) -> List[Row]:
    rows: List[Row] = []
    
    current_degree = None
    seq_time = None
    simd_time = None
    speedup = None

    if not path.exists():
        print(f"Error: Results file not found at {path}")
        sys.exit(1)

    print(f"Parsing results from: {path}")

    with path.open(encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
                

            if line.startswith("Testing degree ="):

                if current_degree is not None and seq_time is not None and simd_time is not None:
                    rows.append((current_degree, seq_time, simd_time, speedup))

                    seq_time = None
                    simd_time = None
                    speedup = None
                
                try:

                    current_degree = int(line.split("=")[1].strip())
                except (IndexError, ValueError):
                    continue

            elif line.startswith("Sequential multiplication average:"):
                try:
                    val_str = line.split(":")[1].strip().split()[0]
                    seq_time = float(val_str)
                except (IndexError, ValueError):
                    pass


            elif line.startswith("SIMD Avg Time:"):
                try:
                    val_str = line.split(":")[1].strip().split()[0]
                    simd_time = float(val_str)
                except (IndexError, ValueError):
                    pass


            elif line.startswith("Speedup (Seq/SIMD):"):
                try:
                    val_str = line.split(":")[1].strip().replace("x", "")
                    if "N/A" not in val_str:
                        speedup = float(val_str)
                    else:
                        speedup = 0.0
                except (IndexError, ValueError):
                    pass


    if current_degree is not None and seq_time is not None and simd_time is not None:
        rows.append((current_degree, seq_time, simd_time, speedup))

    if not rows:
        print(f"Warning: No valid summary data found in {path}.")
        print("Make sure the C program finished successfully and printed the '===== AVERAGES =====' section.")
        sys.exit(1)
        
    return rows

def plot(rows: List[Row], output_dir: pathlib.Path) -> None:
  
    output_dir.mkdir(parents=True, exist_ok=True)
    

    rows.sort(key=lambda x: x[0])
    
    degrees = [r[0] for r in rows]
    seq_times = [r[1] for r in rows]
    simd_times = [r[2] for r in rows]
    speedups = [r[3] if r[3] is not None else 0.0 for r in rows]

    # --- Plot 1: Execution Time vs Degree ---
    plt.figure(figsize=(10, 6))
    
    plt.plot(degrees, seq_times, marker='o', label='Sequential', linestyle='-', color='#d62728', linewidth=2)
    plt.plot(degrees, simd_times, marker='s', label='SIMD (AVX2)', linestyle='-', color='#2ca02c', linewidth=2)
    
    plt.title("Performance Comparison: Sequential vs SIMD", fontsize=14)
    plt.xlabel("Polynomial Degree (N)", fontsize=12)
    plt.ylabel("Execution Time (seconds)", fontsize=12)
    plt.legend(fontsize=12)
    plt.grid(True, linestyle="--", alpha=0.6)
    
    plt.gca().get_xaxis().set_major_formatter(plt.FuncFormatter(lambda x, loc: "{:,}".format(int(x))))
    
    plt.tight_layout()
    out_file_time = output_dir / "simd_execution_time.png"
    plt.savefig(out_file_time)
    plt.close()
    print(f"Generated plot: {out_file_time}")

    # --- Plot 2: Speedup vs Degree ---
    plt.figure(figsize=(10, 6))
    

    x_labels = [f"{d:,}" for d in degrees]
    bars = plt.bar(x_labels, speedups, color='#1f77b4', edgecolor='black', width=0.5)
    
    plt.title("Speedup Achieved (Sequential / SIMD)", fontsize=14)
    plt.xlabel("Polynomial Degree (N)", fontsize=12)
    plt.ylabel("Speedup Factor (x)", fontsize=12)
    

    top_limit = max(speedups) * 1.2 if speedups else 1
    plt.ylim(bottom=0, top=top_limit)
    plt.grid(axis='y', linestyle="--", alpha=0.6)
    

    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height + 0.05,
                 f'{height:.2f}x',
                 ha='center', va='bottom', fontweight='bold', fontsize=11)

    plt.tight_layout()
    out_file_speedup = output_dir / "simd_speedup.png"
    plt.savefig(out_file_speedup)
    plt.close()
    print(f"Generated plot: {out_file_speedup}")

def main() -> None:
    parser = argparse.ArgumentParser(description="Plot SIMD benchmark results")
    parser.add_argument("results", help="Path to results txt file")
    parser.add_argument("--output-dir", required=True, help="Directory to save plots")
    
    args = parser.parse_args()

    results_path = pathlib.Path(args.results)
    output_dir = pathlib.Path(args.output_dir)

    data_rows = parse_results(results_path)
    plot(data_rows, output_dir)

if __name__ == "__main__":
    main()