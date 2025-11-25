#!/bin/bash

# resolve script directory so paths work no matter the invocation point
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

# name of the executable program
PROG="$ROOT_DIR/build/2.out"

OUTPUT_FILE="$RESULTS_DIR/2.txt"

ITERATIONS=(100000 500000 1000000)
THREADS=(2 4 6 8 10 12)

# clear old output file
echo "--------Synchronization Benchmark--------" > "$OUTPUT_FILE"
printf '\n' >> "$OUTPUT_FILE"

for iter in "${ITERATIONS[@]}"; do
    { echo "Testing iterations = $iter"; echo "-------------------------------------"; } >> "$OUTPUT_FILE"
    echo "Testing iterations = $iter"
    echo "-------------------------------------"

    for t in "${THREADS[@]}"; do
        echo "Running: iterations = $iter, threads = $t" | tee -a "$OUTPUT_FILE"
        if [ ! -x "$PROG" ]; then
            echo "Error: executable '$PROG' not found or not executable. Did you run make?" | tee -a "$OUTPUT_FILE"
            exit 1
        fi
        "$PROG" "$iter" "$t" | tee -a "$OUTPUT_FILE"
        printf '\n' >> "$OUTPUT_FILE"
    done

    printf '\n' >> "$OUTPUT_FILE"
done

echo "All tests completed. Results saved in $OUTPUT_FILE"
