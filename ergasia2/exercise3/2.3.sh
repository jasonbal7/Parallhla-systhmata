#!/bin/bash

sizes=(100000 10000000 100000000)  # 10^5, 10^7 and 10^8.
nThreads=(1 2 4 8 16)

runs=4

printf "\n%-12s %-10s %-10s\n" "Array Size" "Threads" "Avg Time(s)"
echo "----------------------------------------"

for N in "${sizes[@]}"; do
    for t in "${nThreads[@]}"; do
        total=0
        for((i=1; i<=runs; i++)); do
            if [ "$t" -eq 1 ]; then       # Serial run
                time=$(./mergesort $N s $t | awk '/Time/{print $(NF-1)}')
            else                         # Parallel run                
                time=$(./mergesort $N p $t | awk '/Time/{print $(NF-1)}')
            fi
            total=$(echo "$total + $time" | bc -l)
        done
        avg=$(echo "$total / $runs" | bc -l)
        printf "%-12d %-10d %-10.3f\n" $N $t $avg
    done
done
