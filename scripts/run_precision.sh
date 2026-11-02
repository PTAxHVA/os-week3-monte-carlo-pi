#!/usr/bin/env bash
# Accuracy experiments (separate from the timing benchmark in run_bench.sh).
#
#   scripts/run_precision.sh           full run (~5-10 min on 4 cores)
#   scripts/run_precision.sh --quick   smoke test (~5 s)
#
# Part A -> results/pi_runs.csv : 5 runs of pi_single / pi_local / pi_mutex at 1e7 points,
#           each run with a DIFFERENT seed (42, 1042, ...). Seeds are 1000 apart because
#           thread tid uses seed+tid: consecutive seeds would reuse 3 of 4 thread streams.
# Part B -> results/precision.csv : pi_big (64-bit xoshiro256+ RNG, 64-bit counters) with
#           N = 1e7 .. 5e11 points on nproc threads; prints pi and its standard error.
#           rand_r cannot do this: its state is 32 bits, so it repeats after ~2e9 points.
set -euo pipefail

usage() { echo "Usage: $0 [--quick]" >&2; exit 1; }
QUICK=0
if [ $# -gt 1 ]; then usage; fi
if [ $# -eq 1 ]; then [ "$1" = "--quick" ] || usage; QUICK=1; fi

cd "$(dirname "$0")/.."
mkdir -p results
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu)
log() { echo "[$(date +%H:%M:%S)] $*" >&2; }

if [ "$QUICK" = 1 ]; then
  N=1000000; SEEDS="42 1042"; BIG_NS="10000000 100000000"
  RUNS_OUT=results/pi_runs-quick.csv; PREC_OUT=results/precision-quick.csv
else
  N=10000000; SEEDS="42 1042 2042 3042 4042"
  BIG_NS="10000000 100000000 1000000000 10000000000 100000000000 500000000000"
  RUNS_OUT=results/pi_runs.csv; PREC_OUT=results/precision.csv
fi

log "build"
make clean > /dev/null
make > /dev/null

log "part A: one run per seed ($SEEDS), N=$N, T=$NPROC"
echo "run,seed,approach,threads,k,points,inside,pi,time_sec" > "$RUNS_OUT"
i=0
for s in $SEEDS; do
  i=$((i + 1))
  for cmd in "./pi_single $N $s" "./pi_local $N $NPROC $s" "./pi_mutex $N $NPROC $s 1000"; do
    echo "$i,$s,$($cmd)" >> "$RUNS_OUT"
  done
done
log "wrote $RUNS_OUT"

log "part B: pi_big, T=$NPROC, N = $BIG_NS"
echo "approach,threads,k,points,inside,pi,time_sec,std_error" > "$PREC_OUT"
for n in $BIG_NS; do
  log "  N=$n"
  ./pi_big "$n" "$NPROC" 42 >> "$PREC_OUT"
done
log "wrote $PREC_OUT"

python3 - "$RUNS_OUT" "$PREC_OUT" <<'PY'
import csv, math, statistics, sys
rows = list(csv.DictReader(open(sys.argv[1])))
print("\nPart A: pi per run (different seeds)")
for a in ("single", "local", "mutex"):
    p = [float(r["pi"]) for r in rows if r["approach"] == a]
    sd = statistics.stdev(p) if len(p) > 1 else 0.0
    print(f"  {a:6s} " + " ".join(f"{x:.6f}" for x in p)
          + f"  mean={statistics.mean(p):.6f} std={sd:.6f}")
print("\nPart B: pi_big")
for r in csv.DictReader(open(sys.argv[2])):
    p = float(r["pi"])
    print(f"  N={int(r['points']):>14,d}  pi={p:.10f}  |err|={abs(p - math.pi):.2e}"
          f"  std_error={r['std_error']}  time={r['time_sec']}s")
PY
