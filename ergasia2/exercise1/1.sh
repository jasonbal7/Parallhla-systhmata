#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"
PLOTS_DIR="$ROOT_DIR/plots"
mkdir -p "$RESULTS_DIR" "$PLOTS_DIR"

PROG="$SCRIPT_DIR/../build/exercise1"
SRC="$SCRIPT_DIR/1.c"
OUTPUT_FILE="$RESULTS_DIR/1.txt"
PLOT_SCRIPT="$SCRIPT_DIR/plot_results.py"

DEGREES=(10000 100000)
THREADS=(2 4 6 8 10 12)
REPEATS=4

# Optional overrides (space-separated), e.g.:
#   DEGREES_OVERRIDE="10000" REPEATS_OVERRIDE=2 ./1.sh
if [ -n "${DEGREES_OVERRIDE:-}" ]; then
    read -r -a DEGREES <<< "$DEGREES_OVERRIDE"
fi
if [ -n "${THREADS_OVERRIDE:-}" ]; then
    read -r -a THREADS <<< "$THREADS_OVERRIDE"
fi
if [ -n "${REPEATS_OVERRIDE:-}" ]; then
    REPEATS="$REPEATS_OVERRIDE"
fi

if [ ! -f "$SRC" ]; then
    echo "Error: source '$SRC' not found." >&2
    exit 1
fi

echo "--------Polynomial Multiplication Benchmark (ergasia2/exercise1)--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

log() {
    # Log to results file and stdout (for progress), without polluting the file with per-run details.
    echo "$1" >> "$OUTPUT_FILE"
    echo "$1"
}

for deg in "${DEGREES[@]}"; do
    log "Testing degree = $deg"
    log "-------------------------------------"
    log "Running: degree1 = $deg, degree2 = $deg, threads = ${THREADS[*]}"
    log ""

    seq_sum=0
    declare -A par_sum
    for th in "${THREADS[@]}"; do
        par_sum[$th]=0
    done

    for ((run=1; run<=REPEATS; run++)); do
        out=$("$PROG" "$deg" "$deg" "${THREADS[@]}")

        seq=$(echo "$out" | grep -E "^Sequential multiplication took" | awk '{print $4}')
        seq_sum=$(echo "$seq_sum + $seq" | bc)

        for th in "${THREADS[@]}"; do
            val=$(echo "$out" | grep -E "^Parallel multiplication with ${th} threads took" | awk '{print $7}')
            par_sum[$th]=$(echo "${par_sum[$th]} + $val" | bc)
        done
    done

    log "===== AVERAGES ====="
    seq_avg=$(echo "scale=4; $seq_sum / $REPEATS" | bc)
    log "Sequential multiplication average: $seq_avg seconds"

    for th in "${THREADS[@]}"; do
        avg=$(echo "scale=4; ${par_sum[$th]} / $REPEATS" | bc)
        log "Parallel multiplication with $th threads average: $avg seconds"
    done

    echo "" >> "$OUTPUT_FILE"
done

if command -v python3 >/dev/null 2>&1 && [ -f "$PLOT_SCRIPT" ]; then
    if MPLBACKEND=Agg python3 "$PLOT_SCRIPT" "$OUTPUT_FILE" --output-dir "$PLOTS_DIR" --table; then
        echo "Generated plots"
    else
        echo "Warning: plotting failed (missing matplotlib?)." | tee -a "$OUTPUT_FILE"
    fi
else
    echo "Plot script missing or python3 unavailable; skipping plots." | tee -a "$OUTPUT_FILE"
fi
