#!/bin/bash

#name of the executable program
PROG="./1"

OUTPUT_FILE="1.txt"

DEGREES=(10000 100000)
THREADS=(2 4 6 8 10 12)

#clear old output file
echo "--------Polynomial Multiplication Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for deg in "${DEGREES[@]}"; do
    # deg1 and deg2 are the same — adjust if needed
    echo "Testing degree = $deg" | tee -a "$OUTPUT_FILE"
    echo "-------------------------------------" | tee -a "$OUTPUT_FILE"

    thread_args=("${THREADS[@]}")
    echo "Running: degree1 = $deg, degree2 = $deg, threads = ${thread_args[*]}" | tee -a "$OUTPUT_FILE"
    $PROG "$deg" "$deg" "${thread_args[@]}" | tee -a "$OUTPUT_FILE"

    echo "" >> "$OUTPUT_FILE"
done

echo "All tests completed. Results saved in $OUTPUT_FILE"