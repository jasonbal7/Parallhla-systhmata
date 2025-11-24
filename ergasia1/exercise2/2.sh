#!/bin/bash

# name of the executable program
PROG="./2"

OUTPUT_FILE="2.txt"

ITERATIONS=(100000 500000 1000000)
THREADS=(2 4 6 8 10 12)

# clear old output file
echo "--------Synchronization Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for iter in "${ITERATIONS[@]}"; do
    echo "Testing iterations = $iter" | tee -a "$OUTPUT_FILE"
    echo "-------------------------------------" | tee -a "$OUTPUT_FILE"

    for t in "${THREADS[@]}"; do
        echo "Running: iterations = $iter, threads = $t" | tee -a "$OUTPUT_FILE"
        $PROG "$iter" "$t" | tee -a "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"
    done

    echo "" >> "$OUTPUT_FILE"
done

echo "All tests completed. Results saved in $OUTPUT_FILE"
