#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"
PLOTS_DIR="$ROOT_DIR/plots"

mkdir -p "$RESULTS_DIR" "$PLOTS_DIR"

PROG="$SCRIPT_DIR/mergesort"
SRC="$SCRIPT_DIR/mergesort.c"
OUTPUT_FILE="$RESULTS_DIR/3.txt"
PLOT_SCRIPT="$SCRIPT_DIR/plot_results.py"

sizes=(100000 10000000 100000000)  # 10^5, 10^7 and 10^8.
nThreads=(1 2 4 8 12)
runs=4

# Optional overrides (space-separated)
if [ -n "${SIZES_OVERRIDE:-}" ]; then
    read -r -a sizes <<< "$SIZES_OVERRIDE"
fi
if [ -n "${THREADS_OVERRIDE:-}" ]; then
    read -r -a nThreads <<< "$THREADS_OVERRIDE"
fi
if [ -n "${RUNS_OVERRIDE:-}" ]; then
    runs="$RUNS_OVERRIDE"
fi

if [ ! -f "$SRC" ]; then
    echo "Error: source '$SRC' not found." >&2
    exit 1
fi

# Rebuild if missing, not executable, or stale.
if [ ! -x "$PROG" ] || [ "$SRC" -nt "$PROG" ]; then
    echo "Building $SRC (requires gcc with OpenMP support)..."
    gcc -std=c11 -O2 -Wall -Wextra -fopenmp "$SRC" -o "$PROG"
fi

{
    printf "%-12s %-10s %-10s\n" "Array Size" "Threads" "Avg Time(s)"
    echo "----------------------------------------"

    for N in "${sizes[@]}"; do
        for t in "${nThreads[@]}"; do
            total=0
            for ((i=1; i<=runs; i++)); do
                if [ "$t" -eq 1 ]; then
                    time=$($PROG "$N" s "$t" | awk '/Time/{print $(NF-1)}')
                else
                    time=$($PROG "$N" p "$t" | awk '/Time/{print $(NF-1)}')
                fi
                total=$(echo "$total + $time" | bc -l)
            done
            avg=$(echo "$total / $runs" | bc -l)
            printf "%-12d %-10d %-10.3f\n" "$N" "$t" "$avg"
        done
    done
} | tee "$OUTPUT_FILE"

if command -v python3 >/dev/null 2>&1 && [ -f "$PLOT_SCRIPT" ]; then
    if MPLBACKEND=Agg python3 "$PLOT_SCRIPT" "$OUTPUT_FILE" --output-dir "$PLOTS_DIR" --table; then
        echo "Generated plots in $PLOTS_DIR"
    else
        echo "Warning: plotting failed." >&2
    fi
else
    echo "Plot script missing or python3 unavailable; skipping plots." >&2
fi
