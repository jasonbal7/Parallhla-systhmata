#!/bin/bash

# resolve project paths
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

# name of the executable program
PROG="$ROOT_DIR/build/3.out"

OUTPUT_FILE="$RESULTS_DIR/3.txt"

SIZES=(10000 50000 100000 250000 100000000)

# clear old output file
echo "--------Array Statistics Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for size in "${SIZES[@]}"; do
    echo "Running: size = $size" | tee -a "$OUTPUT_FILE"
    if [ ! -x "$PROG" ]; then
        echo "Error: executable '$PROG' not found or not executable. Did you run make?" | tee -a "$OUTPUT_FILE"
        exit 1
    fi
    "$PROG" "$size" | tee -a "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
done

echo "All tests completed. Results saved in $OUTPUT_FILE"
