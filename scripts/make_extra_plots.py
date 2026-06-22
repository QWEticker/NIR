#!/usr/bin/env python3
"""
Additional study figures (requested follow-up), built from results/study/*.csv:

  1. trans_vs_distance.png   -- estimated transmissivity T_hat vs distance.
  2. xi_vs_distance.png       -- effective excess noise xi_eff vs distance.
  3. iab_vs_distance.png      -- mutual information I_AB vs distance.
  4. chi_vs_distance.png      -- Holevo bound chi_BE vs distance.
  5. sensitivity_<attack>.png -- stage-3 one-at-a-time parameter sweeps:
        how K_beta / I_AB / chi_BE respond to alpha, beta and the calibration knob.

Run scripts/run_study.py first, then: python scripts/make_extra_plots.py
"""
import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
STUDY = os.path.join(ROOT, "results", "study")
FIGS = os.path.join(STUDY, "figures")
os.makedirs(FIGS, exist_ok=True)

ATTACKS = ["saturation", "lo", "intercept_resend", "collective"]
LABELS = {
    "saturation": "Насыщение детектора",
    "lo": "Манипуляция LO",
    "intercept_resend": "Перехват-пересылка",
    "collective": "Коллективная атака",
}
STYLE = {
    "saturation": ("o", "--", "#1f77b4"),
    "lo": ("s", "--", "#ff7f0e"),
    "intercept_resend": ("D", "--", "#2ca02c"),
    "collective": ("v", "--", "#d62728"),
}
PARAM_LABEL = {
    "alpha": r"Амплитуда $\alpha$ (модуляция $V_A=\alpha^2$)",
    "beta": r"Эффективность согласования $\beta$",
    "saturation_level": "Порог насыщения АЦП",
    "scale": "Калибровка LO (scale)",
}


def load_curve(path, xkey="distance_km"):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    x = [float(r[xkey]) for r in rows]
    cols = {k: [float(r[k]) for r in rows] for k in rows[0] if k != xkey}
    return x, cols


def metric_vs_distance(metric, ylabel, title, fname, logy=False):
    bd, bc = load_curve(os.path.join(STUDY, "baseline.csv"))
    plt.figure(figsize=(8, 5))
    plt.plot(bd, bc[metric], "k^-", lw=2.4, label="Без атаки (baseline)")
    for atk in ATTACKS:
        d, c = load_curve(os.path.join(STUDY, f"attack_{atk}.csv"))
        mk, ls, col = STYLE[atk]
        plt.plot(d, c[metric], marker=mk, ls=ls, color=col, label=LABELS[atk])
    plt.xlabel("Расстояние, км")
    plt.ylabel(ylabel)
    plt.title(title)
    if logy:
        plt.yscale("log")
    plt.grid(alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(FIGS, fname), dpi=140)
    plt.close()


def sensitivity(attack):
    # find the sweep files that exist for this attack
    params = ["alpha", "beta"]
    for extra in ("saturation_level", "scale"):
        if os.path.exists(os.path.join(STUDY, f"sweep_{attack}_{extra}.csv")):
            params.append(extra)
    n = len(params)
    fig, axes = plt.subplots(1, n, figsize=(5 * n, 4.2), squeeze=False)
    for ax, p in zip(axes[0], params):
        x, c = load_curve(os.path.join(STUDY, f"sweep_{attack}_{p}.csv"), xkey=p)
        ax.plot(x, c["I_AB"], "o-", color="#1f77b4", label=r"$I_{AB}$")
        ax.plot(x, c["chi_BE"], "s-", color="#ff7f0e", label=r"$\chi_{BE}$")
        ax.plot(x, c["K_beta"], "^-", color="#2ca02c", lw=2.2, label=r"$K_\beta$")
        ax.axhline(0, color="k", lw=0.6)
        ax.set_xlabel(PARAM_LABEL.get(p, p))
        ax.grid(alpha=0.3)
        if p in ("saturation_level",):
            ax.set_xscale("log")
    axes[0][0].set_ylabel("бит / символ")
    axes[0][0].legend(loc="best", fontsize=9)
    fig.suptitle(f"Этап 3. Чувствительность метрик к параметрам — {LABELS[attack]} "
                 f"(d = 25 км)", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.95))
    fig.savefig(os.path.join(FIGS, f"sensitivity_{attack}.png"), dpi=140)
    plt.close(fig)


def main():
    metric_vs_distance("T_hat", r"$\hat{T}$ (оценка трансмиссивности)",
                       "График 1. Оценка трансмиссивности $\\hat{T}$ vs расстояние",
                       "trans_vs_distance.png")
    metric_vs_distance("xi_eff", r"$\xi_{eff}$, SNU (эффективный избыточный шум)",
                       "График 2. Эффективный избыточный шум $\\xi_{eff}$ vs расстояние",
                       "xi_vs_distance.png")
    metric_vs_distance("I_AB", r"$I_{AB}$, бит / символ",
                       "График 3. Взаимная информация $I_{AB}$ vs расстояние",
                       "iab_vs_distance.png")
    metric_vs_distance("chi_BE", r"$\chi_{BE}$, бит / символ",
                       "График 4. Граница Холево $\\chi_{BE}$ vs расстояние",
                       "chi_vs_distance.png")
    for atk in ATTACKS:
        sensitivity(atk)
    print(f"Figures written to {FIGS}")


if __name__ == "__main__":
    main()
