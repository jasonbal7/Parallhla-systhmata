#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

PROG="$ROOT_DIR/build/3.out"
OUTPUT_FILE="$RESULTS_DIR/3.txt"

SIZES=(10000 50000 100000 250000 100000000)
REPEATS=4

echo "--------Array Statistics Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for size in "${SIZES[@]}"; do

    echo "Testing size = $size" | tee -a "$OUTPUT_FILE"
    echo "----------------------------------" | tee -a "$OUTPUT_FILE"
    echo "" | tee -a "$OUTPUT_FILE"

    init_sum=0
    seq_sum=0
    par_sum=0

    for ((run=1; run<=REPEATS; run++)); do
        echo "Run $run:" | tee -a "$OUTPUT_FILE"

        output=$("$PROG" "$size")

        echo "$output" | tee -a "$OUTPUT_FILE"

        init_time=$(echo "$output" | grep "Arrays creation time" | awk '{print $4}')
        par_time=$(echo "$output" | grep "Parallel computation time" | awk '{print $4}')
        seq_time=$(echo "$output" | grep "Serial computation time" | awk '{print $4}')

        init_sum=$(echo "$init_sum + $init_time" | bc)
        seq_sum=$(echo "$seq_sum + $seq_time" | bc)
        par_sum=$(echo "$par_sum + $par_time" | bc)

        echo "" | tee -a "$OUTPUT_FILE"
    done

    echo "===== AVERAGES =====" | tee -a "$OUTPUT_FILE"

    init_avg=$(echo "scale=4; $init_sum / $REPEATS" | bc)
    seq_avg=$(echo "scale=4; $seq_sum / $REPEATS" | bc)
    par_avg=$(echo "scale=4; $par_sum / $REPEATS" | bc)

    printf "Initialization average time: %.4f seconds\n" "$init_avg" | tee -a "$OUTPUT_FILE"
    printf "Serial average time:        %.4f seconds\n" "$seq_avg" | tee -a "$OUTPUT_FILE"
    printf "Parallel average time:      %.4f seconds\n" "$par_avg" | tee -a "$OUTPUT_FILE"

    echo "" | tee -a "$OUTPUT_FILE"
done

echo "All tests completed. Results saved in $OUTPUT_FILE"
