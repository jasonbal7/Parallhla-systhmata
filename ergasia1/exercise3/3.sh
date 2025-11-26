#!/bin/bash

# Fix for decimal point (force dot instead of comma)
export LC_NUMERIC=C

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

PROG="$ROOT_DIR/build/3.out"
OUTPUT_FILE="$RESULTS_DIR/3c.txt"

SIZES=(10000 50000 100000 250000 100000000)
REPEATS=4

echo "--------Array Statistics Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for size in "${SIZES[@]}"; do

    echo "Testing size = $size" | tee -a "$OUTPUT_FILE"
    echo "-------------------------------------" | tee -a "$OUTPUT_FILE"

    init_sum=0
    seq_sum=0
    par_sum=0

    for ((run=1; run<=REPEATS; run++)); do

        echo "" | tee -a "$OUTPUT_FILE"
        echo "Run $run:" | tee -a "$OUTPUT_FILE"

        if [ ! -x "$PROG" ]; then
            echo "Error: executable '$PROG' not found. Did you run make?" | tee -a "$OUTPUT_FILE"
            exit 1
        fi

        output=$("$PROG" "$size")

        echo "$output" | tee -a "$OUTPUT_FILE"

        init_time=$(echo "$output" | grep "Arrays creation time" | awk '{print $4}')
        par_time=$(echo "$output" | grep "Parallel computation time" | awk '{print $4}')
        seq_time=$(echo "$output" | grep "Serial computation time" | awk '{print $4}')

        init_sum=$(echo "$init_sum + $init_time" | bc)
        seq_sum=$(echo "$seq_sum + $seq_time" | bc)
        par_sum=$(echo "$par_sum + $par_time" | bc)

    done

    echo "" | tee -a "$OUTPUT_FILE"
    echo "===== AVERAGES =====" | tee -a "$OUTPUT_FILE"

    init_avg=$(echo "scale=6; $init_sum / $REPEATS" | bc)
    seq_avg=$(echo "scale=6; $seq_sum / $REPEATS" | bc)
    par_avg=$(echo "scale=6; $par_sum / $REPEATS" | bc)

    echo "Initialization average time:  $init_avg seconds" | tee -a "$OUTPUT_FILE"
    echo "Serial average time:         $seq_avg seconds" | tee -a "$OUTPUT_FILE"
    echo "Parallel average time:       $par_avg seconds" | tee -a "$OUTPUT_FILE"

    echo "" >> "$OUTPUT_FILE"
done

echo "All tests completed. Results saved in $OUTPUT_FILE"
