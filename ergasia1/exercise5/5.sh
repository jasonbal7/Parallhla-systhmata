#!/bin/bash

PROG1="./build/5.1"
PROG2="./build/5.2"
PROG3="./build/5.3"

OUTPUT_FILE="5.txt"

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

