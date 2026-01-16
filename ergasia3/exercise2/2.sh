#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
RESULTS_DIR="$SCRIPT_DIR/results"
PLOTS_DIR="$SCRIPT_DIR/plots"

mkdir -p "$RESULTS_DIR" "$PLOTS_DIR"

PROG="$SCRIPT_DIR/2"
SRC="$SCRIPT_DIR/2.c"
OUTPUT_FILE="$RESULTS_DIR/results_mpi.txt"
PLOT_SCRIPT="$SCRIPT_DIR/plot_results.py"

SIZES=(1000 10000)
SPARSITIES=(0.20 0.50 0.80)
ITERATIONS=(10 20)
PROCESSES=(1 2 4 8)
REPEATS=4

# --- Overrides ---
if [ -n "${SIZES_OVERRIDE:-}" ]; then read -r -a SIZES <<< "$SIZES_OVERRIDE"; fi
if [ -n "${SPARSITIES_OVERRIDE:-}" ]; then read -r -a SPARSITIES <<< "$SPARSITIES_OVERRIDE"; fi
if [ -n "${ITERATIONS_OVERRIDE:-}" ]; then read -r -a ITERATIONS <<< "$ITERATIONS_OVERRIDE"; fi
if [ -n "${PROCESSES_OVERRIDE:-}" ]; then read -r -a PROCESSES <<< "$PROCESSES_OVERRIDE"; fi
if [ -n "${REPEATS_OVERRIDE:-}" ]; then REPEATS="$REPEATS_OVERRIDE"; fi

# --- Compilation ---
if [ ! -x "$PROG" ] || [ "$SRC" -nt "$PROG" ]; then
  if [ -f "$SRC" ]; then
    echo "Building $SRC using mpicc..."
    mpicc -Wall "$SRC" -o "$PROG"
    if [ $? -ne 0 ]; then
        echo "Error: Compilation failed!"
        exit 1
    fi
  else
    echo "Error: executable '$PROG' not found and source '$SRC' missing." >&2
    exit 1
  fi
fi

log() {
  echo "$1" >> "$OUTPUT_FILE"
  echo "$1"
}

# --- Initialize Result File ---
{
  echo "-------- Sparse Matrix MPI Benchmark (Final Version) --------"
  echo ""
  echo "Date: $(date)"
  echo "Each case reports averages over REPEATS=$REPEATS runs."
  echo ""
} > "$OUTPUT_FILE"

# --- Main Loops ---
for n in "${SIZES[@]}"; do
  for sp in "${SPARSITIES[@]}"; do
    for it in "${ITERATIONS[@]}"; do
      
      log "Testing N = $n, Sparsity = $sp, Iterations = $it"
      log "-------------------------------------"

      for p in "${PROCESSES[@]}"; do
        log "MPI Processes = $p"

        sum_construct=0
        sum_comm_csr=0
        sum_calc_csr=0
        sum_total_csr=0
        
        sum_calc_dense=0
        sum_total_dense=0

        for ((run=1; run<=REPEATS; run++)); do
          out=$(mpiexec -f machines -n "$p" "$PROG" "$n" "$sp" "$it")

          # CSR Timings
          val_construct=$(echo "$out" | grep "Construction Time CSR" | awk -F '=' '{print $2}' | awk '{print $1}')
          val_comm_csr=$(echo "$out" | grep "Communication Time CSR" | awk -F '=' '{print $2}' | awk '{print $1}')
          val_calc_csr=$(echo "$out" | grep "Calculation Time CSR" | awk -F '=' '{print $2}' | awk '{print $1}')
          val_total_csr=$(echo "$out" | grep "Total CSR time" | awk -F '=' '{print $2}' | awk '{print $1}')
          
          # Dense Timings
          val_calc_dense=$(echo "$out" | grep "Calculation Time Dense" | awk -F '=' '{print $2}' | awk '{print $1}')
          val_total_dense=$(echo "$out" | grep "Total Dense Time" | awk -F '=' '{print $2}' | awk '{print $1}')

          # Accumulate values
          sum_construct=$(echo "$sum_construct + $val_construct" | bc)
          sum_comm_csr=$(echo "$sum_comm_csr + $val_comm_csr" | bc)
          sum_calc_csr=$(echo "$sum_calc_csr + $val_calc_csr" | bc)
          sum_total_csr=$(echo "$sum_total_csr + $val_total_csr" | bc)
          
          sum_calc_dense=$(echo "$sum_calc_dense + $val_calc_dense" | bc)
          sum_total_dense=$(echo "$sum_total_dense + $val_total_dense" | bc)
        done

        # Calculate Averages
        avg_construct=$(echo "scale=6; $sum_construct / $REPEATS" | bc)
        avg_comm_csr=$(echo "scale=6; $sum_comm_csr / $REPEATS" | bc)
        avg_calc_csr=$(echo "scale=6; $sum_calc_csr / $REPEATS" | bc)
        avg_total_csr=$(echo "scale=6; $sum_total_csr / $REPEATS" | bc)
        
        avg_calc_dense=$(echo "scale=6; $sum_calc_dense / $REPEATS" | bc)
        avg_total_dense=$(echo "scale=6; $sum_total_dense / $REPEATS" | bc)

        # Log Averages (Format for Python script)
        log "===== AVERAGES ====="
        log "(i)   Avg CSR Construction Time : $avg_construct sec"
        log "(ii)  Avg CSR Comm Time         : $avg_comm_csr sec"
        log "(iii) Avg CSR Calculation Time  : $avg_calc_csr sec"
        log "(iv)  Avg Dense Calculation Time: $avg_calc_dense sec"
        log "(v)   Avg TOTAL CSR Time        : $avg_total_csr sec"
        log "(vi)  Avg TOTAL DENSE Time      : $avg_total_dense sec"
        log ""
      done
      log ""
    done
  done
done

# --- Plotting ---
if command -v python3 >/dev/null 2>&1 && [ -f "$PLOT_SCRIPT" ]; then
  echo "Generating plots in $PLOTS_DIR..."
  if MPLBACKEND=Agg python3 "$PLOT_SCRIPT" "$OUTPUT_FILE" --output-dir "$PLOTS_DIR"; then
    echo "Plots generated successfully."
  else
    echo "Warning: Plotting failed." | tee -a "$OUTPUT_FILE"
  fi
else
  echo "Skipping plots ($PLOT_SCRIPT missing or python3 not found)." | tee -a "$OUTPUT_FILE"
fi

echo "Done. Results saved in $OUTPUT_FILE"