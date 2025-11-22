#!/bin/bash

# name of the executable program
PROG="./3"

OUTPUT_FILE="3.txt"

SIZES=(10000 50000 100000 250000 100000000)

# clear old output file
echo "--------Array Statistics Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for size in "${SIZES[@]}"; do
    echo "Running: size = $size" | tee -a "$OUTPUT_FILE"
    $PROG "$size" | tee -a "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
done

echo "All tests completed. Results saved in $OUTPUT_FILE"
