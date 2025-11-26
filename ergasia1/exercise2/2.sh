#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

PROG="$ROOT_DIR/build/2.out"
OUTPUT_FILE="$RESULTS_DIR/2c.txt"

ITERATIONS=(100000 500000 1000000)
THREADS=(2 4 6 8 10 12)
REPEATS=4

echo "--------Synchronization Benchmark--------" > "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

for iter in "${ITERATIONS[@]}"; do

    echo "Testing iterations = $iter" | tee -a "$OUTPUT_FILE"
    echo "-------------------------------------" | tee -a "$OUTPUT_FILE"

    for th in "${THREADS[@]}"; do

        echo "Running: iterations = $iter, threads = $th" | tee -a "$OUTPUT_FILE"

        mutex_sum=0
        rwlock_sum=0
        atomic_sum=0

        for ((run=1; run<=REPEATS; run++)); do
            echo "" | tee -a "$OUTPUT_FILE"
            echo "Run $run:" | tee -a "$OUTPUT_FILE"

            if [ ! -x "$PROG" ]; then
                echo "Error: executable '$PROG' not found. Did you run make?" | tee -a "$OUTPUT_FILE"
                exit 1
            fi

            output=$("$PROG" "$iter" "$th")

            echo "$output" | tee -a "$OUTPUT_FILE"

            mutex_time=$(echo "$output" | grep "Elapsed time with mutex" | awk '{print $6}')
            rwlock_time=$(echo "$output" | grep "Elapsed time with rwlock" | awk '{print $5}')
            atomic_time=$(echo "$output" | grep "Elapsed time with atomic" | awk '{print $6}')

            mutex_sum=$(echo "$mutex_sum + $mutex_time" | bc)
            rwlock_sum=$(echo "$rwlock_sum + $rwlock_time" | bc)
            atomic_sum=$(echo "$atomic_sum + $atomic_time" | bc)
        done

        echo "" | tee -a "$OUTPUT_FILE"
        echo "===== AVERAGES =====" | tee -a "$OUTPUT_FILE"

        mutex_avg=$(echo "scale=6; $mutex_sum / $REPEATS" | bc)
        rwlock_avg=$(echo "scale=6; $rwlock_sum / $REPEATS" | bc)
        atomic_avg=$(echo "scale=6; $atomic_sum / $REPEATS" | bc)

        echo "Mutex average time:  $mutex_avg seconds" | tee -a "$OUTPUT_FILE"
        echo "RWLock average time: $rwlock_avg seconds" | tee -a "$OUTPUT_FILE"
        echo "Atomic average time: $atomic_avg seconds" | tee -a "$OUTPUT_FILE"
        echo "" >> "$OUTPUT_FILE"

    done

    echo "" >> "$OUTPUT_FILE"
done
