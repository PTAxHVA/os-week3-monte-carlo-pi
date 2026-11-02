#!/usr/bin/env bash
# Run every benchmark configuration of the Week 3 lab and write one CSV.
#
#   scripts/run_bench.sh           full run  -> results/raw.csv + results/machine.txt
#   scripts/run_bench.sh --quick   smoke test (small N, 1 run) -> results/raw-quick.csv
#                                  (also rewrites results/machine.txt; the full run rewrites it again)
#
# Order (so Task 2 and Task 5 get numbers early):
#   1. pi_single            : 1 warm-up + RUNS recorded
#   2. pi_local  x threads  : 1 warm-up + RUNS recorded   -> "single+local done" is printed
#   3. pi_mutex  x threads  : k = 1, RUNS recorded (no warm-up: slow)
#   4. pi_mutex  T = nproc  : k = 10 .. 100000, RUNS recorded (experiment 6.1)
# Progress goes to stderr; the CSV only gets program output lines.
set -euo pipefail

usage() { echo "Usage: $0 [--quick]" >&2; exit 1; }

MODE=full
if [ $# -gt 1 ]; then usage; fi
if [ $# -eq 1 ]; then
  [ "$1" = "--quick" ] || usage
  MODE=quick
fi

cd "$(dirname "$0")/.."
mkdir -p results

# Number of CPUs the OS can schedule on (Linux: nproc, macOS fallback: sysctl).
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu)

SEED=42
if [ "$MODE" = full ]; then
  N=10000000
  RUNS=5
  THREADS="1 2 4 8 16 32 64 128"
  KS="10 100 1000 10000 100000"
  OUT=results/raw.csv
else
  N=1000000
  RUNS=1
  THREADS="1 2 4"
  KS="10 1000"
  OUT=results/raw-quick.csv
fi
# Add nproc to the thread list when it is not already there.
case " $THREADS " in *" $NPROC "*) ;; *) THREADS="$THREADS $NPROC" ;; esac
THREADS=$(printf '%s\n' $THREADS | sort -n | uniq | tr '\n' ' ')

log() { echo "[$(date +%H:%M:%S)] $*" >&2; }

# Run one program once; check the output is a single 7-field CSV line.
run_once() {
  local line
  line=$("$@")
  if [ "$(printf '%s' "$line" | awk -F, '{print NF}')" != 7 ]; then
    log "ERROR: unexpected output from '$*': $line"
    exit 1
  fi
  printf '%s\n' "$line"
}

# run_config <warmup 0|1> <command...>: optional warm-up, then RUNS recorded runs.
run_config() {
  local warm=$1; shift
  if [ "$warm" = 1 ]; then run_once "$@" > /dev/null; fi
  for _ in $(seq "$RUNS"); do run_once "$@" >> "$OUT"; done
}

log "build"
make clean > /dev/null
make > /dev/null

{
  echo "date: $(date '+%Y-%m-%d %H:%M:%S %z')"
  echo "nproc: $NPROC"
  echo "virtualization: $(systemd-detect-virt 2>/dev/null || echo unknown)"
  echo "uname: $(uname -a)"
  echo "compiler: $(${CC:-gcc} --version | head -1)"
  echo "flags: -O2 -Wall -pthread"
  echo "nPoints: $N  seed: $SEED  runs: $RUNS"
  echo
  lscpu 2>/dev/null || sysctl -n machdep.cpu.brand_string 2>/dev/null || true
} > results/machine.txt
log "wrote results/machine.txt (nproc=$NPROC)"

echo "approach,threads,k,points,inside,pi,time_sec" > "$OUT"

log "pi_single"
run_config 1 ./pi_single "$N" "$SEED"

for t in $THREADS; do
  log "pi_local T=$t"
  run_config 1 ./pi_local "$N" "$t" "$SEED"
done
log "single+local done -> $OUT (send these numbers to Task 2 and Task 5 now)"

for t in $THREADS; do
  log "pi_mutex T=$t k=1"
  run_config 0 ./pi_mutex "$N" "$t" "$SEED" 1
done

for k in $KS; do
  log "pi_mutex T=$NPROC k=$k"
  run_config 0 ./pi_mutex "$N" "$NPROC" "$SEED" "$k"
done

log "done: $(($(wc -l < "$OUT") - 1)) rows in $OUT"
