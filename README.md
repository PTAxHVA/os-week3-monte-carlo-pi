# Week 3: Monte Carlo π estimation with threads

Group lab for CO2017 Operating Systems. Three C programs estimate π = 4 × (points inside circle) / (total points) and compare a single-threaded baseline, a multi-threaded local-accumulation version and a multi-threaded shared-counter version protected by a mutex.

Task split, shared conventions and deadlines: [`docs/phan-cong-5-dau-viec.md`](docs/phan-cong-5-dau-viec.md). Read section 2 of that file before writing code.

## Layout

| Path | Owner | Content |
|---|---|---|
| `src/common.h` | Task 1 | `now_sec`, `rand01`, `print_result` |
| `src/pi_single.c` | Task 1 | Approach 1, single thread |
| `src/pi_local.c` | Task 2 | Approach 2, per-thread local counters |
| `src/pi_mutex.c` | Task 3 | Approach 3, shared counter + mutex, optional `k` |
| `Makefile` | Task 1 | `all`, `clean` |
| `scripts/run_bench.sh` | Task 4 | Runs every configuration, writes `results/raw.csv` |
| `scripts/plot.py` | Task 4 | Writes `results/summary.csv` and `figures/*.png` |
| `results/` | Task 4 | `machine.txt`, `raw.csv`, `summary.csv` |
| `figures/` | Task 4 | Plots |
| `report.pdf` | Task 5 | Final report |

Each person edits only the files they own.

## Workflow: branch, pull request, review

`main` is protected. Nobody pushes to it directly:

1. `git switch main && git pull`, then `git switch -c task<N>-<topic>` (for example `task2-pi-local`).
2. Commit on that branch, then `git push -u origin HEAD`.
3. Open a pull request into `main` and request a review from **@EigenKai (Tuấn Anh)**.
4. Tuấn Anh reviews and merges. Do not merge your own PR.

AI assistants must follow the same rules; they are written out in [`AGENTS.md`](AGENTS.md), and `CLAUDE.md`, `GEMINI.md` and `.github/copilot-instructions.md` point to it.

## Build and run

### Prerequisites

- GCC with C11 support
- POSIX threads (`-pthread`)
- Python 3 with `pandas` and `matplotlib` (for plotting only)

### Build

```bash
make all        # builds pi_single, pi_local, pi_mutex
make clean      # removes the three binaries
```

All three programs are compiled with `gcc -O2 -Wall -pthread`.

### Run

Each program prints **one CSV line** to stdout (no header):

```
approach,threads,k,points,inside,pi,time_sec
```

```bash
# Approach 1 — single thread (baseline)
./pi_single 10000000 42

# Approach 2 — local counters, 4 threads
./pi_local 10000000 4 42

# Approach 3 — shared counter with mutex, 4 threads, k defaults to 1
./pi_mutex 10000000 4 42

# Approach 3 — mutex, batch size k = 1000
./pi_mutex 10000000 4 42 1000
```

Wrong or missing arguments print a usage message to stderr and exit with code 1.

- `pi_single` and `pi_local` always output `k=0`.
- `pi_mutex` outputs the `k` value passed on the command line (default `1`).

### Benchmark

```bash
bash scripts/run_bench.sh      # writes results/raw.csv
python3 scripts/plot.py         # writes results/summary.csv and figures/*.png
```

All official measurements run on one machine (Task 4). See `results/machine.txt` for hardware details.
