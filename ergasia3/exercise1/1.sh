#!/bin/csh -f

# Polynomial multiplication benchmark runner (MPI)
# Supports:
#   mpiexec -f machines -n <p> <program>
#   mpiexec -hosts linux01,linux02 -n <p> <program>

set script_dir = `dirname "$0"`
set script_dir = `cd "$script_dir" && pwd`
set root_dir = `cd "$script_dir/.." && pwd`

set results_dir = "$root_dir/results"
set plots_dir = "$root_dir/plots"
mkdir -p "$results_dir" "$plots_dir"

set src = "$script_dir/1.c"
set prog = "$root_dir/build/1"
set output_file = "$results_dir/1.txt"
set plot_script = "$script_dir/plot_results.py"

alias log 'echo "\!:*" | tee -a "$output_file"'

set DEGREES = (10000 100000)
set PROCS = (2 4 8 16)
set REPEATS = 4

set MPI_MACHINES_FILE = "$script_dir/machines"
set MPI_HOSTS = ""

set usage_exit = 0
goto MAIN

USAGE:
cat << EOF
Usage: ./1.sh [options]

Options:
    --machines-file <path>          Hostfile path (default: ./machines)
    --hosts <h1,h2,...>             Host list (if provided, runs with -hosts)
    -h, --help                      Show this help

Env vars are not used.
EOF
exit $usage_exit

MAIN:

while ( $#argv > 0 )
    switch ("$argv[1]")
        case "--machines-file":
            if ( $#argv < 2 ) then
                echo "Error: --machines-file needs a value" >&2
                set usage_exit = 2
                goto USAGE
            endif
            set MPI_MACHINES_FILE = "$argv[2]"
            shift argv
            shift argv
            breaksw
        case "--hosts":
            if ( $#argv < 2 ) then
                echo "Error: --hosts needs a value" >&2
                set usage_exit = 2
                goto USAGE
            endif
            set MPI_HOSTS = "$argv[2]"
            shift argv
            shift argv
            breaksw
        case "-h":
        case "--help":
            set usage_exit = 0
            goto USAGE
        default:
            echo "Unknown option: $argv[1]" >&2
            set usage_exit = 2
            goto USAGE
    endsw
end

set MPIEXEC_OPTS = ()
if ( "$MPI_HOSTS" == "" ) then
    if ( ! -f "$MPI_MACHINES_FILE" ) then
        echo "Error: MPI machines file '$MPI_MACHINES_FILE' not found." >&2
        exit 1
    endif
    set MPIEXEC_OPTS = ( $MPIEXEC_OPTS -f "$MPI_MACHINES_FILE" )
else
    set MPIEXEC_OPTS = ( $MPIEXEC_OPTS -hosts "$MPI_HOSTS" )
endif

if ( ! -f "$src" ) then
    echo "Error: source '$src' not found." >&2
    exit 1
endif
if ( ! -x "$prog" ) then
    echo "Error: program '$prog' not found or not executable." >&2
    echo "Build it first (e.g., run 'make' in ergasia3)." >&2
    exit 1
endif

echo "--------Polynomial Multiplication Benchmark (ergasia3/exercise1 MPI)--------" >! "$output_file"
echo "" >> "$output_file"

foreach deg ( $DEGREES )
    log "Testing degree = $deg"
    log "-------------------------------------"
    log "Running: degree = $deg, processes = $PROCS"
    log ""

    set seq_sum = 0
    set send_sum = ()
    set comp_sum = ()
    set recv_sum = ()
    set total_sum = ()

    @ idx = 1
    while ( $idx <= $#PROCS )
        set send_sum = ( $send_sum 0 )
        set comp_sum = ( $comp_sum 0 )
        set recv_sum = ( $recv_sum 0 )
        set total_sum = ( $total_sum 0 )
        @ idx++
    end

    @ run = 1
    while ( $run <= $REPEATS )
        @ idx = 1
        foreach p ( $PROCS )
            set tmp = `mktemp`
            set cmd = ( mpiexec $MPIEXEC_OPTS -n $p "$prog" $deg )

            $cmd >&! "$tmp"
            if ( $status != 0 ) then
                echo "Error: run failed for degree=$deg p=$p" >&2
                cat "$tmp" >&2
                /bin/rm -f "$tmp"
                exit 1
            endif

            # Sequential time: take from first process-count only
            if ( $idx == 1 ) then
                set seq = `awk '/^Sequential time:/{for(i=1;i<=NF;i++) if($i ~ /^[0-9]*\.?[0-9]+$/){print $i; exit}}' "$tmp"`
                if ( "$seq" == "" ) then
                    echo "Error: failed to parse sequential time for degree=$deg" >&2
                    cat "$tmp" >&2
                    /bin/rm -f "$tmp"
                    exit 1
                endif
                set seq_sum = `echo "$seq_sum $seq" | awk '{printf "%.6f", $1+$2}'`
            endif

            # Parallel timings
            set send = `awk '/^(Time to send data:|Time to send slices:)/{for(i=1;i<=NF;i++) if($i ~ /^[0-9]*\.?[0-9]+$/){print $i; exit}}' "$tmp"`
            set comp = `awk '/^(Time for parallel computation:|Parallel computation:)/{for(i=1;i<=NF;i++) if($i ~ /^[0-9]*\.?[0-9]+$/){print $i; exit}}' "$tmp"`
            set recv = `awk '/^(Time to receive results:|Time to gather results:)/{for(i=1;i<=NF;i++) if($i ~ /^[0-9]*\.?[0-9]+$/){print $i; exit}}' "$tmp"`
            set total = `awk '/^(Total parallel time:|Total parallel:)/{for(i=1;i<=NF;i++) if($i ~ /^[0-9]*\.?[0-9]+$/){print $i; exit}}' "$tmp"`

            if ( "$send" == "" || "$comp" == "" || "$recv" == "" || "$total" == "" ) then
                echo "Error: failed to parse timings for degree=$deg p=$p" >&2
                cat "$tmp" >&2
                /bin/rm -f "$tmp"
                exit 1
            endif

            set send_sum[$idx] = `echo "$send_sum[$idx] $send" | awk '{printf "%.6f", $1+$2}'`
            set comp_sum[$idx] = `echo "$comp_sum[$idx] $comp" | awk '{printf "%.6f", $1+$2}'`
            set recv_sum[$idx] = `echo "$recv_sum[$idx] $recv" | awk '{printf "%.6f", $1+$2}'`
            set total_sum[$idx] = `echo "$total_sum[$idx] $total" | awk '{printf "%.6f", $1+$2}'`

            /bin/rm -f "$tmp"
            @ idx++
        end
        @ run++
    end

    log "===== AVERAGES ====="
    set seq_avg = `echo "$seq_sum $REPEATS" | awk '{printf "%.6f", $1/$2}'`
    log "Sequential multiplication average: $seq_avg seconds"

    @ idx = 1
    foreach p ( $PROCS )
        set total_avg = `echo "$total_sum[$idx] $REPEATS" | awk '{printf "%.6f", $1/$2}'`
        set send_avg = `echo "$send_sum[$idx] $REPEATS" | awk '{printf "%.6f", $1/$2}'`
        set comp_avg = `echo "$comp_sum[$idx] $REPEATS" | awk '{printf "%.6f", $1/$2}'`
        set recv_avg = `echo "$recv_sum[$idx] $REPEATS" | awk '{printf "%.6f", $1/$2}'`

        log "Parallel multiplication with $p processes average: $total_avg seconds"
        log "  Send average: $send_avg seconds"
        log "  Compute average: $comp_avg seconds"
        log "  Receive average: $recv_avg seconds"
        log ""
        @ idx++
    end

    echo "" >> "$output_file"
end

which python3 >& /dev/null
if ( $status == 0 && -f "$plot_script" ) then
    setenv MPLBACKEND Agg
    python3 "$plot_script" "$output_file" --output-dir "$plots_dir" --table
    if ( $status == 0 ) then
        echo "Generated plots in $plots_dir"
    else
        echo "Warning: plotting failed (missing matplotlib?)." | tee -a "$output_file"
    endif
else
    echo "Plot script missing or python3 unavailable; skipping plots." | tee -a "$output_file"
endif
