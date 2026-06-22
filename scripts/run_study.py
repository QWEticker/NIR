#!/usr/bin/env python3
"""
End-to-end security study for the cvqkd simulator.

Stages
------
1. Baseline  : run the optimal operating point (no attack) over a distance sweep.
2. Attacks   : run each attack (saturation, LO manipulation, intercept-resend,
               collective) at the same optimal parameters.
3. Recovery  : for every attack, search over system parameters (modulation
               variance V_A = alpha^2, reconciliation efficiency beta, and the
               relevant calibration knob) and keep, per distance, the setting
               that maximises the secret-key rate K_beta -- i.e. try to make the
               attacked system behave like the un-attacked one.

All raw results are written as CSV under results/study/ so they can be plotted
by scripts/make_plots.py.

Usage:
    python scripts/run_study.py [path-to-cvqkd_sim(.exe)]
"""
import json
import os
import sys
import csv
import subprocess
import itertools

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUTDIR = os.path.join(ROOT, "results", "study")
TMPDIR = os.path.join(OUTDIR, "_tmp")
os.makedirs(TMPDIR, exist_ok=True)


def find_exe():
    if len(sys.argv) > 1:
        return sys.argv[1]
    for cand in ["build-mingw/cvqkd_sim.exe", "build/cvqkd_sim.exe",
                 "build-mingw/cvqkd_sim", "build/cvqkd_sim",
                 "bin/cvqkd_sim.exe", "bin/cvqkd_sim"]:
        p = os.path.join(ROOT, cand)
        if os.path.exists(p):
            return p
    raise SystemExit("cvqkd_sim executable not found; pass its path as argv[1]")


EXE = find_exe()

# ---------------------------------------------------------------------------
# Optimal operating point (see experiments/optimal.json for literature refs).
# ---------------------------------------------------------------------------
DISTANCES = [1, 5, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80]
OPT = dict(alpha=2.0, xi=0.01, eta=0.6, v_el=0.015, beta=0.95,
           saturation_level=1e9, N=600000, seed=42)
SEARCH_N = 150000  # smaller N for the (many) recovery-search runs

# Attack definitions at the optimal point.
ATTACKS = {
    "saturation":       {"name": "saturation", "saturation_level": 2.0, "gain": 1.0},
    "lo":               {"name": "lo", "scale": 0.85},
    "intercept_resend": {"name": "intercept_resend", "meas_eff": 0.85, "resend_gain": 1.0},
    "collective":       {"name": "collective", "coupling": 0.3, "excess_noise": 0.05},
}


def make_cfg(name, base, distances, attack_obj):
    return {
        "name": name,
        "base": {k: base[k] for k in
                 ("alpha", "xi", "eta", "v_el", "beta", "saturation_level", "N", "seed")},
        "distances_km": distances,
        "attacks": [attack_obj] if attack_obj is not None else ["none"],
        "runs_per_point": 1,
    }


def run_cfg(cfg, tag):
    cpath = os.path.join(TMPDIR, f"cfg_{tag}.json")
    opath = os.path.join(TMPDIR, f"out_{tag}.csv")
    with open(cpath, "w") as f:
        json.dump(cfg, f)
    subprocess.run([EXE, "--config", cpath, "--out", opath],
                   check=True, stdout=subprocess.DEVNULL)
    with open(opath) as f:
        return list(csv.DictReader(f))


CURVE_COLS = ("T_true", "T_hat", "xi_hat", "xi_eff", "I_AB", "chi_BE", "K_asymp", "K_beta")


def rows_to_curve(rows):
    """dist -> dict of floats"""
    out = {}
    for r in rows:
        out[float(r["distance_km"])] = {k: float(r[k]) for k in CURVE_COLS}
    return out


def write_curve(path, dist_to_metrics, extra_cols=None):
    extra_cols = extra_cols or {}
    cols = ["distance_km", *CURVE_COLS]
    ekeys = sorted({k for d in extra_cols.values() for k in d})
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(cols + ekeys)
        for d in sorted(dist_to_metrics):
            m = dist_to_metrics[d]
            row = [d] + [m[c] for c in cols[1:]]
            ex = extra_cols.get(d, {})
            row += [ex.get(k, "") for k in ekeys]
            w.writerow(row)


def main():
    print(f"Using executable: {EXE}")

    # --- Stage 1: baseline ---------------------------------------------------
    print("\n[1] Baseline (no attack) at optimal parameters ...")
    base_rows = run_cfg(make_cfg("baseline", OPT, DISTANCES, None), "baseline")
    base_curve = rows_to_curve(base_rows)
    write_curve(os.path.join(OUTDIR, "baseline.csv"), base_curve)
    for d in DISTANCES:
        m = base_curve[d]
        print(f"    d={d:5.0f} km  I_AB={m['I_AB']:.3f}  chi_BE={m['chi_BE']:.3f}  "
              f"K_beta={m['K_beta']:.4f}")

    # --- Stage 2: attacks at optimal params ---------------------------------
    print("\n[2] Attacks at optimal parameters ...")
    attacked = {}
    for key, atk in ATTACKS.items():
        rows = run_cfg(make_cfg(f"attack_{key}", OPT, DISTANCES, atk), f"atk_{key}")
        attacked[key] = rows_to_curve(rows)
        write_curve(os.path.join(OUTDIR, f"attack_{key}.csv"), attacked[key])
        kb0 = attacked[key][DISTANCES[0]]["K_beta"]
        kb_base0 = base_curve[DISTANCES[0]]["K_beta"]
        print(f"    {key:18s}  K_beta@{DISTANCES[0]}km = {kb0:.4f}  "
              f"(baseline {kb_base0:.4f})")

    # --- Stage 3: recovery by parameter search ------------------------------
    print("\n[3] Recovery: searching system parameters per attack ...")
    # generic knobs available to the legitimate users
    ALPHA_GRID = [1.0, 1.5, 2.0, 2.5, 3.0]
    BETA_GRID = [0.95, 0.98]
    # attack-specific calibration knob (defence)
    CALIB = {
        "saturation":       ("saturation_level", [2.0, 4.0, 8.0, 1e9]),
        "lo":               ("scale", [0.85, 0.92, 0.97, 1.0]),
        "intercept_resend": (None, [None]),
        "collective":       (None, [None]),
    }

    for key, atk in ATTACKS.items():
        calib_name, calib_vals = CALIB[key]
        # best K_beta and chosen params per distance
        best = {d: {"K_beta": -1.0} for d in DISTANCES}
        best_params = {d: {} for d in DISTANCES}
        combos = list(itertools.product(ALPHA_GRID, BETA_GRID, calib_vals))
        print(f"    {key}: {len(combos)} parameter combinations ...")
        for (alpha, beta, calib) in combos:
            base = dict(OPT)
            base["alpha"] = alpha
            base["beta"] = beta
            base["N"] = SEARCH_N
            atk_obj = dict(atk)
            if calib_name is not None and calib is not None:
                atk_obj[calib_name] = calib
                if calib_name == "saturation_level":
                    base["saturation_level"] = calib
            tag = f"rec_{key}_a{alpha}_b{beta}_c{calib}"
            rows = run_cfg(make_cfg(f"rec_{key}", base, DISTANCES, atk_obj), tag)
            curve = rows_to_curve(rows)
            for d in DISTANCES:
                if curve[d]["K_beta"] > best[d]["K_beta"]:
                    best[d] = curve[d]
                    bp = {"alpha": alpha, "V_A": alpha * alpha, "beta": beta}
                    if calib_name is not None and calib is not None:
                        bp[calib_name] = calib
                    best_params[d] = bp
        write_curve(os.path.join(OUTDIR, f"recovered_{key}.csv"), best, best_params)
        # short summary
        d0 = DISTANCES[0]
        bp = best_params[d0]
        print(f"      best@{d0}km K_beta={best[d0]['K_beta']:.4f} "
              f"(baseline {base_curve[d0]['K_beta']:.4f}) params={bp}")

    # --- Stage 4: one-at-a-time parameter sensitivity -----------------------
    # At a fixed reference distance, vary ONE knob at a time (others at optimal)
    # and record how each metric responds. This isolates the contribution of
    # each parameter to the recovery searched in stage 3.
    print("\n[4] Parameter sensitivity (one knob at a time) ...")
    D_REF = 25.0
    SWEEPS = {
        "alpha": [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0],
        "beta":  [0.90, 0.92, 0.94, 0.95, 0.96, 0.98, 1.0],
    }
    CALIB_SWEEP = {
        "saturation": ("saturation_level", [1.5, 2.0, 3.0, 4.0, 8.0, 1e9]),
        "lo":         ("scale", [0.80, 0.85, 0.90, 0.95, 0.97, 1.0]),
    }

    def sweep_one(key, atk, pname, values, calib_name=None):
        out = {}
        for v in values:
            base = dict(OPT)
            base["N"] = SEARCH_N
            atk_obj = dict(atk)
            if pname in ("alpha", "beta"):
                base[pname] = v
            elif calib_name is not None:
                atk_obj[calib_name] = v
                if calib_name == "saturation_level":
                    base["saturation_level"] = v
            tag = f"sw_{key}_{pname}_{v}"
            rows = run_cfg(make_cfg(f"sw_{key}", base, [D_REF], atk_obj), tag)
            out[v] = rows_to_curve(rows)[D_REF]
        return out

    for key, atk in ATTACKS.items():
        params = dict(SWEEPS)
        if key in CALIB_SWEEP:
            cname, cvals = CALIB_SWEEP[key]
            params[cname] = cvals
        for pname, values in params.items():
            cname = CALIB_SWEEP[key][0] if (key in CALIB_SWEEP and pname == CALIB_SWEEP[key][0]) else None
            res = sweep_one(key, atk, pname, values, cname)
            path = os.path.join(OUTDIR, f"sweep_{key}_{pname}.csv")
            with open(path, "w", newline="") as f:
                w = csv.writer(f)
                w.writerow([pname, *CURVE_COLS])
                for v in values:
                    m = res[v]
                    w.writerow([v] + [m[c] for c in CURVE_COLS])
            print(f"    {key}/{pname}: {len(values)} points @ {D_REF:.0f} km -> {os.path.basename(path)}")

    print(f"\nDone. CSVs written to {OUTDIR}")


if __name__ == "__main__":
    main()
