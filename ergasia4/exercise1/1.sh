#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)

RESULTS_DIR="$ROOT_DIR/results"
PLOTS_DIR="$ROOT_DIR/plots"

mkdir -p "$RESULTS_DIR"
mkdir -p "$PLOTS_DIR"

PROG="$SCRIPT_DIR/1"
OUTPUT_FILE="$RESULTS_DIR/1.txt"
PLOT_SCRIPT="$SCRIPT_DIR/plot_results.py"

DEGREES=(50000 100000 500000)
REPEATS=3

echo "--------Polynomial Multiplication SIMD Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for deg in "${DEGREES[@]}"; do

    echo "Testing degree = $deg" | tee -a "$OUTPUT_FILE"
    echo "-------------------------------------" | tee -a "$OUTPUT_FILE"
    echo "Running: degree1 = $deg, degree2 = $deg" | tee -a "$OUTPUT_FILE"

    for ((run=1; run<=REPEATS; run++)); do
        echo "" | tee -a "$OUTPUT_FILE"
        echo "Run $run:" | tee -a "$OUTPUT_FILE"

        if [ ! -x "$PROG" ]; then
            echo "Error: executable '$PROG' not found." | tee -a "$OUTPUT_FILE"
            echo "Please compile with: gcc -O3 -mavx2 -o 1 1.c"
            exit 1
        fi

        "$PROG" "$deg" "$deg" | tee -a "$OUTPUT_FILE"
    done

    echo "" | tee -a "$OUTPUT_FILE"
    echo "===== AVERAGES =====" | tee -a "$OUTPUT_FILE"

    seq_sum=0

    seq_times=$(grep "Sequential multiplication took" "$OUTPUT_FILE" | tail -"$REPEATS" | awk '{print $4}')

    for val in $seq_times; do
        seq_sum=$(echo "$seq_sum + $val" | bc)
    done

    seq_avg=$(echo "scale=4; $seq_sum / $REPEATS" | bc)
    echo "Sequential multiplication average: $seq_avg seconds" | tee -a "$OUTPUT_FILE"


    simd_sum=0
    simd_times=$(grep "SIMD multiplication took" "$OUTPUT_FILE" | tail -"$REPEATS" | awk '{print $4}')
    
    for val in $simd_times; do
        simd_sum=$(echo "$simd_sum + $val" | bc)
    done
    
    simd_avg=$(echo "scale=6; $simd_sum / $REPEATS" | bc)
    echo "SIMD Avg Time:       $simd_avg seconds" | tee -a "$OUTPUT_FILE"


    if (( $(echo "$simd_avg > 0" | bc -l) )); then
        speedup=$(echo "scale=2; $seq_avg / $simd_avg" | bc)
        echo "Speedup (Seq/SIMD):  ${speedup}x" | tee -a "$OUTPUT_FILE"
    else
        echo "Speedup: N/A (too fast)" | tee -a "$OUTPUT_FILE"
    fi

    echo "" >> "$OUTPUT_FILE"
    echo "-------------------------------------" >> "$OUTPUT_FILE"
done

echo "Benchmarks completed. Results saved in $OUTPUT_FILE"


if command -v python3 >/dev/null 2>&1; then
    if [ -f "$PLOT_SCRIPT" ]; then
        echo "Generating plots..."
        python3 "$PLOT_SCRIPT" "$OUTPUT_FILE" --output-dir "$PLOTS_DIR"
        echo "Plots saved in $PLOTS_DIR"
    else
        echo "Warning: Plot script '$PLOT_SCRIPT' not found."
    fi
else
    echo "Warning: python3 not found, skipping plot generation."
fi