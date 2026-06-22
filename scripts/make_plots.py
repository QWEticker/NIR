#!/usr/bin/env python3
"""
Build the figures for the three-stage CV-QKD security study from the CSVs
produced by scripts/run_study.py (results/study/*.csv).

Figures (written to results/study/figures/):
  1. baseline_metrics.png   -- I_AB, chi_BE, K_beta vs distance (no attack).
  2. attacks_kbeta.png       -- K_beta vs distance: baseline + every attack.
  3. recovery_<attack>.png   -- per attack: baseline vs attacked vs recovered.
  4. summary_kbeta.png       -- grid of the four recovery panels.

Usage:  python scripts/make_plots.py
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


def load(path):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    d = [float(r["distance_km"]) for r in rows]
    cols = {}
    for k in ("I_AB", "chi_BE", "K_beta", "K_asymp", "xi_hat"):
        cols[k] = [float(r[k]) for r in rows]
    return d, cols


def main():
    base_d, base = load(os.path.join(STUDY, "baseline.csv"))

    # --- Figure 1: baseline security metrics ------------------------------
    plt.figure(figsize=(8, 5))
    plt.plot(base_d, base["I_AB"], "o-", label=r"$I_{AB}$ (взаимная информация)")
    plt.plot(base_d, base["chi_BE"], "s-", label=r"$\chi_{BE}$ (граница Холево)")
    plt.plot(base_d, base["K_beta"], "^-", lw=2.2, label=r"$K_\beta$ (секретный ключ)")
    plt.axhline(0, color="k", lw=0.6)
    plt.xlabel("Расстояние, км")
    plt.ylabel("бит / символ")
    plt.title("Этап 1. Оптимальные параметры без атак")
    plt.legend()
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(FIGS, "baseline_metrics.png"), dpi=140)
    plt.close()

    # --- Figure 2: attacks vs baseline (K_beta) ---------------------------
    plt.figure(figsize=(8, 5))
    plt.plot(base_d, base["K_beta"], "k^-", lw=2.4, label="Без атаки (baseline)")
    markers = ["o", "s", "D", "v"]
    for atk, mk in zip(ATTACKS, markers):
        d, c = load(os.path.join(STUDY, f"attack_{atk}.csv"))
        plt.plot(d, c["K_beta"], mk + "--", label=LABELS[atk])
    plt.axhline(0, color="k", lw=0.6)
    plt.xlabel("Расстояние, км")
    plt.ylabel(r"$K_\beta$, бит / символ")
    plt.title("Этап 2. Деградация секретного ключа под атаками")
    plt.legend()
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(FIGS, "attacks_kbeta.png"), dpi=140)
    plt.close()

    # --- Figure 3 + 4: recovery -------------------------------------------
    fig, axes = plt.subplots(2, 2, figsize=(12, 8), sharex=True, sharey=True)
    for ax, atk in zip(axes.ravel(), ATTACKS):
        ad, ac = load(os.path.join(STUDY, f"attack_{atk}.csv"))
        rd, rc = load(os.path.join(STUDY, f"recovered_{atk}.csv"))

        # standalone figure
        plt.figure(figsize=(8, 5))
        plt.plot(base_d, base["K_beta"], "k^-", lw=2.2, label="Без атаки")
        plt.plot(ad, ac["K_beta"], "rs--", label="Под атакой")
        plt.plot(rd, rc["K_beta"], "go-", lw=2.0, label="После подбора параметров")
        plt.axhline(0, color="k", lw=0.6)
        plt.xlabel("Расстояние, км")
        plt.ylabel(r"$K_\beta$, бит / символ")
        plt.title(f"Этап 3. Восстановление: {LABELS[atk]}")
        plt.legend()
        plt.grid(alpha=0.3)
        plt.tight_layout()
        plt.savefig(os.path.join(FIGS, f"recovery_{atk}.png"), dpi=140)
        plt.close()

        # panel in the summary grid
        ax.plot(base_d, base["K_beta"], "k^-", lw=2.0, label="Без атаки")
        ax.plot(ad, ac["K_beta"], "rs--", label="Под атакой")
        ax.plot(rd, rc["K_beta"], "go-", lw=1.8, label="Восстановлено")
        ax.axhline(0, color="k", lw=0.6)
        ax.set_title(LABELS[atk])
        ax.grid(alpha=0.3)
    for ax in axes[-1]:
        ax.set_xlabel("Расстояние, км")
    for ax in axes[:, 0]:
        ax.set_ylabel(r"$K_\beta$, бит / символ")
    axes[0, 0].legend(loc="upper right", fontsize=9)
    fig.suptitle("Сводка: baseline vs атака vs восстановление", fontsize=13)
    fig.tight_layout(rect=(0, 0, 1, 0.97))
    fig.savefig(os.path.join(FIGS, "summary_kbeta.png"), dpi=140)
    plt.close(fig)

    print(f"Figures written to {FIGS}")


if __name__ == "__main__":
    main()
