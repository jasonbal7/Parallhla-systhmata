#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"
PLOTS_DIR="$ROOT_DIR/plots"

mkdir -p "$RESULTS_DIR" "$PLOTS_DIR"

PROG="$SCRIPT_DIR/2"
SRC="$SCRIPT_DIR/2.c"
OUTPUT_FILE="$RESULTS_DIR/2.txt"
PLOT_SCRIPT="$SCRIPT_DIR/plot_results.py"

# Tunable experiment parameters (kept modest so it runs quickly).
SIZES=(5000 10000)
SPARSITIES=(0.20 0.50 0.80)
ITERATIONS=(10 20)
THREADS=(4 8 12)
REPEATS=4

# Optional overrides (space-separated), e.g.:
#   SIZES_OVERRIDE="1000 5000 10000" ITERATIONS_OVERRIDE="10" REPEATS_OVERRIDE=2 ./2.sh
if [ -n "${SIZES_OVERRIDE:-}" ]; then
  read -r -a SIZES <<< "$SIZES_OVERRIDE"
fi
if [ -n "${SPARSITIES_OVERRIDE:-}" ]; then
  read -r -a SPARSITIES <<< "$SPARSITIES_OVERRIDE"
fi
if [ -n "${ITERATIONS_OVERRIDE:-}" ]; then
  read -r -a ITERATIONS <<< "$ITERATIONS_OVERRIDE"
fi
if [ -n "${THREADS_OVERRIDE:-}" ]; then
  read -r -a THREADS <<< "$THREADS_OVERRIDE"
fi
if [ -n "${REPEATS_OVERRIDE:-}" ]; then
  REPEATS="$REPEATS_OVERRIDE"
fi

if [ ! -x "$PROG" ]; then
  if [ -f "$SRC" ]; then
    echo "Building $SRC (requires gcc with OpenMP support)..."
    gcc -std=c11 -O2 -Wall -Wextra -fopenmp "$SRC" -o "$PROG"
  else
    echo "Error: executable '$PROG' not found and no source '$SRC' to build it." >&2
    exit 1
  fi
fi

log() {
  # Log to results file and stdout (for progress), without writing per-run details.
  echo "$1" >> "$OUTPUT_FILE"
  echo "$1"
}

{
  echo "--------Sparse Matrix (CSR) Benchmark (ergasia2/exercise2)--------"
  echo ""
  echo "Each case reports averages over REPEATS=$REPEATS runs."
  echo ""
} > "$OUTPUT_FILE"

for m in "${SIZES[@]}"; do
  for sp in "${SPARSITIES[@]}"; do
    for it in "${ITERATIONS[@]}"; do
      log "Testing m = $m sparsity = $sp iterations = $it"
      log "-------------------------------------"

      for th in "${THREADS[@]}"; do
        log "Threads = $th"

        serial_conv_sum=0
        parallel_conv_sum=0
        csr_serial_sum=0
        csr_parallel_sum=0
        dense_serial_sum=0
        dense_parallel_sum=0

        for ((run=1; run<=REPEATS; run++)); do
          out=$("$PROG" "$m" "$sp" "$it" "$th")

          serial_conv=$(echo "$out" | grep -E "^Serial conversion to CSR" | awk '{print $(NF-1)}')
          parallel_conv=$(echo "$out" | grep -E "^Parallel conversion to CSR" | awk '{print $(NF-1)}')
          csr_serial=$(echo "$out" | grep -E "^CSR serial multiplication took" | awk '{print $(NF-1)}')
          csr_parallel=$(echo "$out" | grep -E "^CSR parallel multiplication took" | awk '{print $(NF-1)}')
          dense_serial=$(echo "$out" | grep -E "^Dense serial multiplication took" | awk '{print $(NF-1)}')
          dense_parallel=$(echo "$out" | grep -E "^Dense parallel multiplication took" | awk '{print $(NF-1)}')

          serial_conv_sum=$(echo "$serial_conv_sum + $serial_conv" | bc)
          parallel_conv_sum=$(echo "$parallel_conv_sum + $parallel_conv" | bc)
          csr_serial_sum=$(echo "$csr_serial_sum + $csr_serial" | bc)
          csr_parallel_sum=$(echo "$csr_parallel_sum + $csr_parallel" | bc)
          dense_serial_sum=$(echo "$dense_serial_sum + $dense_serial" | bc)
          dense_parallel_sum=$(echo "$dense_parallel_sum + $dense_parallel" | bc)
        done

        log "===== AVERAGES ====="
        serial_conv_avg=$(echo "scale=6; $serial_conv_sum / $REPEATS" | bc)
        parallel_conv_avg=$(echo "scale=6; $parallel_conv_sum / $REPEATS" | bc)
        csr_serial_avg=$(echo "scale=6; $csr_serial_sum / $REPEATS" | bc)
        csr_parallel_avg=$(echo "scale=6; $csr_parallel_sum / $REPEATS" | bc)
        dense_serial_avg=$(echo "scale=6; $dense_serial_sum / $REPEATS" | bc)
        dense_parallel_avg=$(echo "scale=6; $dense_parallel_sum / $REPEATS" | bc)

        log "Serial CSR conversion average: $serial_conv_avg seconds"
        log "Parallel CSR conversion average: $parallel_conv_avg seconds"
        log "CSR serial multiplication average: $csr_serial_avg seconds"
        log "CSR parallel multiplication average: $csr_parallel_avg seconds"
        log "Dense serial multiplication average: $dense_serial_avg seconds"
        log "Dense parallel multiplication average: $dense_parallel_avg seconds"
        log ""
      done

      log ""
    done
  done
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
