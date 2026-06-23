#!/usr/bin/env python3
"""
Generate channel-parameter sweep data for section 6.2
(influence of channel parameters T and xi on the key metrics).

Outputs (results/study/):
  channel_xi_at25km.csv     -- metrics vs excess noise xi at fixed d = 25 km.
  channel_kbeta_vs_dist_xi.csv -- K_beta vs distance for a family of xi values.

Run:  python scripts/gen_channel_sweep.py [path-to-cvqkd_sim(.exe)]
(make sure the mingw runtime is on PATH).
"""
import csv
import itertools
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUTDIR = os.path.join(ROOT, "results", "study")
TMPDIR = os.path.join(OUTDIR, "_tmp")
os.makedirs(TMPDIR, exist_ok=True)


def find_exe():
    if len(sys.argv) > 1:
        return sys.argv[1]
    for cand in ["build-mingw/cvqkd_sim.exe", "build/cvqkd_sim.exe",
                 "build-mingw/cvqkd_sim", "build/cvqkd_sim"]:
        p = os.path.join(ROOT, cand)
        if os.path.exists(p):
            return p
    raise SystemExit("cvqkd_sim executable not found; pass its path as argv[1]")


EXE = find_exe()

OPT = dict(alpha=2.0, eta=0.6, v_el=0.015, beta=0.95,
           saturation_level=1e9, N=400000, seed=42)
DISTANCES = [1, 5, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80]


def run(base, distances, tag):
    cfg = {
        "name": tag,
        "base": {k: base[k] for k in
                 ("alpha", "xi", "eta", "v_el", "beta", "saturation_level", "N", "seed")},
        "distances_km": distances,
        "attacks": ["none"],
        "runs_per_point": 1,
    }
    cpath = os.path.join(TMPDIR, f"cfg_{tag}.json")
    opath = os.path.join(TMPDIR, f"out_{tag}.csv")
    with open(cpath, "w") as f:
        json.dump(cfg, f)
    subprocess.run([EXE, "--config", cpath, "--out", opath],
                   check=True, stdout=subprocess.DEVNULL)
    with open(opath) as f:
        return list(csv.DictReader(f))


COLS = ("T_true", "T_hat", "xi_hat", "xi_eff", "I_AB", "chi_BE", "K_asymp", "K_beta")

# --- xi sweep at fixed distance -------------------------------------------
XI_VALUES = [0.0, 0.005, 0.01, 0.02, 0.03, 0.05, 0.08, 0.1, 0.15, 0.2]
D_REF = 25.0
print(f"[1] xi sweep at d = {D_REF} km ...")
with open(os.path.join(OUTDIR, "channel_xi_at25km.csv"), "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["xi", *COLS])
    for xi in XI_VALUES:
        base = dict(OPT, xi=xi)
        rows = run(base, [D_REF], f"xi_{xi}")
        r = rows[0]
        w.writerow([xi] + [r[c] for c in COLS])
        print(f"    xi={xi:5.3f}  K_beta={float(r['K_beta']):.4f}  "
              f"chi_BE={float(r['chi_BE']):.4f}  I_AB={float(r['I_AB']):.4f}")

# --- K_beta vs distance for a family of xi --------------------------------
XI_FAMILY = [0.005, 0.01, 0.02, 0.05, 0.1]
print("[2] K_beta vs distance for a family of xi ...")
fam = {}
for xi in XI_FAMILY:
    rows = run(dict(OPT, xi=xi), DISTANCES, f"fam_{xi}")
    fam[xi] = {float(r["distance_km"]): float(r["K_beta"]) for r in rows}
    print(f"    xi={xi:5.3f}  K_beta@1km={fam[xi][1]:.4f}  K_beta@50km={fam[xi][50]:.4f}")
with open(os.path.join(OUTDIR, "channel_kbeta_vs_dist_xi.csv"), "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["distance_km", *[f"xi_{xi}" for xi in XI_FAMILY]])
    for d in DISTANCES:
        w.writerow([d] + [f"{fam[xi][d]:.6f}" for xi in XI_FAMILY])

print("Done.")
