#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"
PLOTS_DIR="$ROOT_DIR/plots"
mkdir -p "$RESULTS_DIR" "$PLOTS_DIR"

SRC="$SCRIPT_DIR/1.c"
PROG="$SCRIPT_DIR/1"
OUTPUT_FILE="$RESULTS_DIR/1.txt"
PLOT_SCRIPT="$SCRIPT_DIR/plot_results.py"

#Defaults like previous ergasies
DEGREES=(10000 100000)
PROCS=(2 4 10 16)
REPEATS=4

if [ -n "${DEGREES_OVERRIDE:-}" ]; then
    read -r -a DEGREES <<< "$DEGREES_OVERRIDE"
fi
if [ -n "${PROCS_OVERRIDE:-}" ]; then
    read -r -a PROCS <<< "$PROCS_OVERRIDE"
fi
if [ -n "${REPEATS_OVERRIDE:-}" ]; then
    REPEATS="$REPEATS_OVERRIDE"
fi

if [ ! -f "$SRC" ]; then
    echo "Error: source '$SRC' not found." >&2
    exit 1
fi

# Build (rebuild if missing or source newer)
if [ ! -x "$PROG" ] || [ "$SRC" -nt "$PROG" ]; then
    echo "Building with mpicc..."
    CFLAGS_DEFAULT="-O3 -march=native -mtune=native -funroll-loops -pipe"
    CFLAGS_USE="${CFLAGS:-$CFLAGS_DEFAULT}"
    mpicc $CFLAGS_USE -Wall -Wextra -o "$PROG" "$SRC"
fi

echo "--------Polynomial Multiplication Benchmark (ergasia3/exercise1 MPI)--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

log() {
    echo "$1" >> "$OUTPUT_FILE"
    echo "$1"
}

extract_first_number() {
    # Usage: extract_first_number "<regex>" "<text>"
    # Prints the first numeric token (e.g. 0.123, .123, 1) from the first line matching regex.
    local pattern="$1"
    local text="$2"
    echo "$text" | awk -v pat="$pattern" '
        $0 ~ pat {
            for (i = 1; i <= NF; i++) {
                if ($i ~ /^[0-9]*\.?[0-9]+$/) { print $i; exit }
            }
        }
    '
}

for deg in "${DEGREES[@]}"; do
    log "Testing degree = $deg"
    log "-------------------------------------"
    log "Running: degree = $deg, processes = ${PROCS[*]}"
    log ""

    # Deterministic seed per degree so all runs use identical polynomials.
    SEED=$((1000 + deg))

    # Track averages for each process count
    seq_sum=0
    declare -A par_sum send_sum comp_sum recv_sum
    for p in "${PROCS[@]}"; do
        par_sum[$p]=0
        send_sum[$p]=0
        comp_sum[$p]=0
        recv_sum[$p]=0
    done

    for ((run=1; run<=REPEATS; run++)); do
        for p in "${PROCS[@]}"; do
            outp=$(mpiexec -n "$p" "$PROG" "$deg" "$SEED")

            # Use the sequential time from the first process-count run per repeat.
            if [ "$p" -eq "${PROCS[0]}" ]; then
                seq=$(extract_first_number '^Sequential time:' "$outp")
                if [ -z "$seq" ]; then
                    echo "Error: failed to parse sequential time for degree=$deg" >&2
                    echo "$outp" >&2
                    exit 1
                fi
                seq_sum=$(echo "$seq_sum + $seq" | bc)
            fi

            # Support both label styles:
            # - Newer: "Time to send data:", "Time for parallel computation:", "Time to receive results:", "Total parallel time:"
            # - Older: "Time to send slices:", "Parallel computation:", "Time to gather results:", "Total parallel:"
            send=$(extract_first_number '^(Time to send data:|Time to send slices:)' "$outp")
            comp=$(extract_first_number '^(Time for parallel computation:|Parallel computation:)' "$outp")
            recv=$(extract_first_number '^(Time to receive results:|Time to gather results:)' "$outp")
            total=$(extract_first_number '^(Total parallel time:|Total parallel:)' "$outp")

            if [ -z "$send" ] || [ -z "$comp" ] || [ -z "$recv" ] || [ -z "$total" ]; then
                echo "Error: failed to parse timings for degree=$deg p=$p" >&2
                echo "$outp" >&2
                exit 1
            fi

            send_sum[$p]=$(echo "${send_sum[$p]} + $send" | bc)
            comp_sum[$p]=$(echo "${comp_sum[$p]} + $comp" | bc)
            recv_sum[$p]=$(echo "${recv_sum[$p]} + $recv" | bc)
            par_sum[$p]=$(echo "${par_sum[$p]} + $total" | bc)
        done
    done

    log "===== AVERAGES ====="
    seq_avg=$(echo "scale=6; $seq_sum / $REPEATS" | bc)
    log "Sequential multiplication average: $seq_avg seconds"

    for p in "${PROCS[@]}"; do
        total_avg=$(echo "scale=6; ${par_sum[$p]} / $REPEATS" | bc)
        send_avg=$(echo "scale=6; ${send_sum[$p]} / $REPEATS" | bc)
        comp_avg=$(echo "scale=6; ${comp_sum[$p]} / $REPEATS" | bc)
        recv_avg=$(echo "scale=6; ${recv_sum[$p]} / $REPEATS" | bc)

        log "Parallel multiplication with $p processes average: $total_avg seconds"
        log "  Send average: $send_avg seconds"
        log "  Compute average: $comp_avg seconds"
        log "  Receive average: $recv_avg seconds"
    done

    echo "" >> "$OUTPUT_FILE"

done

if command -v python3 >/dev/null 2>&1 && [ -f "$PLOT_SCRIPT" ]; then
    if MPLBACKEND=Agg python3 "$PLOT_SCRIPT" "$OUTPUT_FILE" --output-dir "$PLOTS_DIR" --table; then
        echo "Generated plots in $PLOTS_DIR"
    else
        echo "Warning: plotting failed (missing matplotlib?)." | tee -a "$OUTPUT_FILE"
    fi
else
    echo "Plot script missing or python3 unavailable; skipping plots." | tee -a "$OUTPUT_FILE"
fi
