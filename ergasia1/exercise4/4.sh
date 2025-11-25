#!/bin/bash

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
RESULTS_DIR="$ROOT_DIR/results"

mkdir -p "$RESULTS_DIR"

PROG="$ROOT_DIR/build/4.out"
OUT="$RESULTS_DIR/4.txt"
PLOT_SCRIPT="$SCRIPT_DIR/plot_results.py"
RUNS=4

SIZES=(1000000 5000000 10000000)
TPS=(2500 5000 10000)
PCTS=(30)
LOCKS=("mutex" "rwlock")
THREADS=(4)

header_border="+---------+----------+-----------------+-------+--------+---------+-----------------+"
{
    printf "%s\n" "$header_border"
    printf "| %-7s | %-8s | %-15s | %-5s | %-6s | %-7s | %-15s |\n" \
        "threads" "accounts" "txns_per_thread" "pct" "lock" "variant" "mean_time (s)"
    printf "%s\n" "$header_border"
} > "$OUT"

for size in "${SIZES[@]}"; do
for tpt in "${TPS[@]}"; do
for pct in "${PCTS[@]}"; do
for lock in "${LOCKS[@]}"; do
for th in "${THREADS[@]}"; do

    total_cg=0
    total_fg=0

    for run in $(seq 1 $RUNS); do
        if [ ! -x "$PROG" ]; then
            echo "Error: executable '$PROG' not found or not executable. Did you run make?" >&2
            exit 1
        fi

        output=$("$PROG" "$size" "$tpt" "$pct" "$lock" "$th")
        mapfile -t times < <(printf '%s\n' "$output" | awk '/^TIME /{print $2}')

        if [ "${#times[@]}" -lt 2 ]; then
            echo "Warning: failed to read both coarse and fine times for size=$size tpt=$tpt pct=$pct lock=$lock threads=$th" >&2
            continue 2
        fi

        cg_time=${times[0]}
        fg_time=${times[1]}

        total_cg=$(echo "$total_cg + $cg_time" | bc -l)
        total_fg=$(echo "$total_fg + $fg_time" | bc -l)
    done

    avg_cg=$(echo "$total_cg / $RUNS" | bc -l)
    avg_fg=$(echo "$total_fg / $RUNS" | bc -l)

    printf "| %7d | %8d | %15d | %5d | %-6s | %-7s | %17.6f |\n" \
        "$th" "$size" "$tpt" "$pct" "$lock" "coarse" "$avg_cg" >> "$OUT"
    printf "| %7d | %8d | %15d | %5d | %-6s | %-7s | %17.6f |\n" \
        "$th" "$size" "$tpt" "$pct" "$lock" "fine" "$avg_fg" >> "$OUT"

done
done
done
done
done

printf "%s\n" "$header_border" >> "$OUT"

echo "Experiments complete. Results in $OUT"

if command -v python3 >/dev/null 2>&1; then
    if [ -f "$PLOT_SCRIPT" ]; then
        if python3 "$PLOT_SCRIPT" "$OUT" --output-dir "$SCRIPT_DIR/plots"; then
            echo "Generated plots in $SCRIPT_DIR/plots."
        else
            echo "Skipping plot generation (matplotlib missing or error occurred)." >&2
        fi
    else
        echo "Plot script '$PLOT_SCRIPT' not found; skipping plot generation." >&2
    fi
else
    echo "Python3 not found; skipping plot generation." >&2
fi
