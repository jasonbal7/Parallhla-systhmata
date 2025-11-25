#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

PROG="$ROOT_DIR/build/1.out"
OUTPUT_FILE="$RESULTS_DIR/1.txt"

DEGREES=(10000 100000)
THREADS=(2 4 6 8 10 12)
REPEATS=4

echo "--------Polynomial Multiplication Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for deg in "${DEGREES[@]}"; do

    echo "Testing degree = $deg" | tee -a "$OUTPUT_FILE"
    echo "-------------------------------------" | tee -a "$OUTPUT_FILE"

    echo "Running: degree1 = $deg, degree2 = $deg, threads = ${THREADS[*]}" | tee -a "$OUTPUT_FILE"

    for ((run=1; run<=REPEATS; run++)); do
        echo "" | tee -a "$OUTPUT_FILE"
        echo "Run $run:" | tee -a "$OUTPUT_FILE"

        if [ ! -x "$PROG" ]; then
            echo "Error: executable '$PROG' not found. Did you run make?" | tee -a "$OUTPUT_FILE"
            exit 1
        fi

        "$PROG" "$deg" "$deg" "${THREADS[@]}" | tee -a "$OUTPUT_FILE"
    done

    echo "" | tee -a "$OUTPUT_FILE"
    echo "===== AVERAGES =====" | tee -a "$OUTPUT_FILE"

    # Υπολογισμός μέσων όρων
    seq_sum=0

    seq_times=$(grep "Sequential multiplication took" "$OUTPUT_FILE" | tail -"$REPEATS" | awk '{print $4}')

    for val in $seq_times; do
        seq_sum=$(echo "$seq_sum + $val" | bc)
    done

    seq_avg=$(echo "scale=4; $seq_sum / $REPEATS" | bc)
    echo "Sequential multiplication average: $seq_avg seconds" | tee -a "$OUTPUT_FILE"

    for th in "${THREADS[@]}"; do
        sum=0

        par_times=$(grep "Parallel multiplication with $th threads took" "$OUTPUT_FILE" | tail -"$REPEATS" | awk '{print $7}')

        for val in $par_times; do
            sum=$(echo "$sum + $val" | bc)
        done

        avg=$(echo "scale=4; $sum / $REPEATS" | bc)

        echo "Parallel multiplication with $th threads average: $avg seconds" | tee -a "$OUTPUT_FILE"
    done

    echo "" >> "$OUTPUT_FILE"
done
