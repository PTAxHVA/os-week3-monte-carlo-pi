#!/usr/bin/env python3
"""Summarise results/raw.csv and draw the four figures of the Week 3 lab.

Usage:
    python3 scripts/plot.py [RAW_CSV] [OUT_DIR]

Defaults: RAW_CSV = results/raw.csv, OUT_DIR = repository root.
Writes OUT_DIR/results/summary.csv and OUT_DIR/figures/{time,speedup,efficiency,sync_k}.png.
The core count is read from results/machine.txt ("nproc: N").
"""
import math
import re
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import pandas as pd  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
EXPECTED_RUNS = 5
PI_TOLERANCE = 0.002


def read_nproc(machine_file: Path) -> int:
    """Return the core count recorded by run_bench.sh."""
    match = re.search(r"^nproc:\s*(\d+)", machine_file.read_text(), re.MULTILINE)
    if not match:
        sys.exit(f"error: no 'nproc:' line in {machine_file}")
    return int(match.group(1))


def summarise(raw: pd.DataFrame) -> pd.DataFrame:
    """Mean/std per (approach, threads, k), plus speedup and efficiency."""
    summary = (
        raw.groupby(["approach", "threads", "k"])
        .agg(runs=("time_sec", "size"), time_mean=("time_sec", "mean"),
             time_std=("time_sec", "std"), pi_mean=("pi", "mean"),
             inside_first=("inside", "first"))
        .reset_index()
    )
    singles = summary[summary.approach == "single"]
    if singles.empty:
        sys.exit("error: no 'single' rows in the raw data")
    t_single = singles.time_mean.iloc[0]
    summary["speedup"] = t_single / summary.time_mean
    summary["efficiency"] = summary.speedup / summary.threads
    return summary


def check(raw: pd.DataFrame, summary: pd.DataFrame) -> None:
    """Print warnings for missing runs or inaccurate pi values."""
    for row in summary.itertuples():
        if row.runs != EXPECTED_RUNS:
            print(f"warning: {row.approach} T={row.threads} k={row.k}: "
                  f"{row.runs} runs (expected {EXPECTED_RUNS})", file=sys.stderr)
    bad = raw[(raw.pi - math.pi).abs() >= PI_TOLERANCE]
    if not bad.empty:
        print(f"warning: {len(bad)} rows with |pi - 3.14159| >= {PI_TOLERANCE}", file=sys.stderr)


def style(ax, title: str, xlabel: str, ylabel: str, note: str) -> None:
    ax.set_title(f"{title}\n{note}", fontsize=10)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)


def thread_axis(ax, threads, nproc: int) -> None:
    ax.set_xscale("log", base=2)
    ax.set_xticks(threads)
    ax.set_xticklabels([str(t) for t in threads])
    ax.axvline(nproc, color="grey", linestyle=":", label=f"nproc = {nproc}")


def plot_all(summary: pd.DataFrame, nproc: int, points: int, fig_dir: Path) -> None:
    runs = int(summary.runs.min())
    note = f"{points:.0e} points, mean of {runs} runs, {nproc} cores"
    single = summary[summary.approach == "single"].iloc[0]
    local = summary[summary.approach == "local"].sort_values("threads")
    mutex = summary[(summary.approach == "mutex") & (summary.k == 1)].sort_values("threads")
    threads = sorted(set(local.threads) | set(mutex.threads))
    series = [("Approach 2: local", local, "o-"), ("Approach 3: mutex (k=1)", mutex, "s-")]

    fig, ax = plt.subplots(figsize=(7, 4.5))
    for label, df, fmt in series:
        ax.errorbar(df.threads, df.time_mean, yerr=df.time_std, fmt=fmt, capsize=3, label=label)
    ax.axhline(single.time_mean, color="black", linestyle="--", label="Approach 1: single thread")
    ax.set_yscale("log")
    thread_axis(ax, threads, nproc)
    style(ax, "Execution time vs number of threads", "Threads", "Time (s, log scale)", note)
    fig.tight_layout(); fig.savefig(fig_dir / "time.png", dpi=150); plt.close(fig)

    fig, ax = plt.subplots(figsize=(7, 4.5))
    for label, df, fmt in series:
        ax.plot(df.threads, df.speedup, fmt, label=label)
    ax.plot(threads, threads, color="grey", linestyle="--", label="Ideal (speedup = N)")
    top = max(nproc, local.speedup.max(), mutex.speedup.max()) * 1.3
    ax.set_ylim(0, top)
    thread_axis(ax, threads, nproc)
    style(ax, "Speedup vs number of threads", "Threads", "Speedup = T_single / T_parallel", note)
    fig.tight_layout(); fig.savefig(fig_dir / "speedup.png", dpi=150); plt.close(fig)

    fig, ax = plt.subplots(figsize=(7, 4.5))
    for label, df, fmt in series:
        ax.plot(df.threads, df.efficiency, fmt, label=label)
    ax.axhline(1.0, color="grey", linestyle="--", label="Ideal (E = 1)")
    thread_axis(ax, threads, nproc)
    style(ax, "Parallel efficiency vs number of threads", "Threads", "Efficiency = speedup / N", note)
    fig.tight_layout(); fig.savefig(fig_dir / "efficiency.png", dpi=150); plt.close(fig)

    sync = summary[(summary.approach == "mutex") & (summary.threads == nproc)].sort_values("k")
    local_n = local[local.threads == nproc]
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.errorbar(sync.k, sync.time_mean, yerr=sync.time_std, fmt="s-", capsize=3,
                label=f"Approach 3: mutex, T = {nproc}")
    if not local_n.empty:
        ax.axhline(local_n.time_mean.iloc[0], color="tab:blue", linestyle="--",
                   label=f"Approach 2: local, T = {nproc}")
    ax.set_xscale("log")
    ax.set_yscale("log")
    style(ax, "Synchronization interval k vs execution time (6.1)",
          "k = iterations between shared updates (log scale)", "Time (s, log scale)", note)
    fig.tight_layout(); fig.savefig(fig_dir / "sync_k.png", dpi=150); plt.close(fig)


def main() -> None:
    if len(sys.argv) > 3:
        sys.exit(__doc__)
    raw_path = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "results/raw.csv"
    out_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT
    raw = pd.read_csv(raw_path)
    nproc = read_nproc(ROOT / "results/machine.txt")

    summary = summarise(raw)
    check(raw, summary)
    (out_dir / "results").mkdir(parents=True, exist_ok=True)
    (out_dir / "figures").mkdir(parents=True, exist_ok=True)
    summary.to_csv(out_dir / "results/summary.csv", index=False, float_format="%.6f")
    plot_all(summary, nproc, int(raw.points.iloc[0]), out_dir / "figures")
    print(f"wrote {out_dir / 'results/summary.csv'} and 4 figures in {out_dir / 'figures'}")


if __name__ == "__main__":
    main()
