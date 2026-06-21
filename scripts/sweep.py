#!/usr/bin/env python3
# Simple wrapper script: runs the cvqkd_sim executable with a given config and saves the output CSV.
import sys, subprocess, os

def run_once(exe, cfg, out):
    cmd = [exe, "--config", cfg, "--output", out]
    print("Running:", " ".join(cmd))
    subprocess.check_call(cmd)

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: sweep.py <path-to-exe> <config.json> <output.csv>")
        sys.exit(1)
    exe, cfg, out = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    run_once(exe, cfg, out)
    print("Done. Output:", out)
