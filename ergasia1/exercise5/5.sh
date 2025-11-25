#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

PROG1="$ROOT_DIR/build/5.1.out"
PROG2="$ROOT_DIR/build/5.2.out"
PROG3="$ROOT_DIR/build/5.3.out"

OUTPUT_FILE="$RESULTS_DIR/5.txt"

THREADS=(2 4 8 16 32);
ITERATIONS=(1000 10000 40000)
RUNS=4

echo "--------Barrier Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

run_test() {
    local prog=$1
    local barrier_type=$2
    local threads=$3
    local iterations=$4

    echo "Running $barrier_type with $threads threads and $iterations iterations" | tee -a "$OUTPUT_FILE"
    if [ ! -x "$prog" ]; then
        echo "Error: executable '$prog' not found or not executable. Did you run make?" | tee -a "$OUTPUT_FILE"
        exit 1
    fi
    local total=0

    for run in $(seq 1 $RUNS); do
        echo " Run $run:" | tee -a "$OUTPUT_FILE"
        result=$($prog "$threads" "$iterations")
        echo "$result" | tee -a "$OUTPUT_FILE"

        time=$(echo "$result" | awk '{print $(NF-1)}')
        total=$(echo "$total + $time" | bc -l)
    done

    avg=$(echo "$total / $RUNS" | bc -l)
    echo "" >> "$OUTPUT_FILE"    
    printf "%s %d %d %.4f\n" "$barrier_type" "$threads" "$iterations" "$avg" | tee -a "$OUTPUT_FILE"
}

for t in "${THREADS[@]}"; do
    for i in "${ITERATIONS[@]}"; do
        run_test "$PROG1" "pthread_barrier (5.1)" "$t" "$i"

        run_test "$PROG2" "mutex_cond_barrier (5.2)" "$t" "$i"

        run_test "$PROG3" "sense_reversal_barrier (5.3)" "$t" "$i"
    done
done

echo "All tests completed. Results saved in $OUTPUT_FILE."

