#!/usr/bin/env python3
"""
Section 6 (РЕЗУЛЬТАТЫ ЭКСПЕРИМЕНТОВ) figures, built from results/study/*.csv.

New figures (written to results/study/report2/figures/):
  fig_6_1_baseline.png       -- 6.1 nominal regime: security metrics vs distance.
  fig_6_1_estimation.png     -- 6.1 nominal regime: channel-estimation quality.
  fig_6_2_family.png         -- 6.2 K_beta vs distance for a family of xi.
  fig_6_2_xi.png             -- 6.2 metrics vs xi at fixed d = 25 km.
  fig_6_5_compare.png        -- 6.5 baseline vs attacks (grouped bars + range).
  fig_6_6_recovery.png       -- 6.6 attack vs recovery (grouped bars + range).
  fig_6_7_applicability.png  -- 6.7 max secure distance across all scenarios.

The four "metric vs distance" plots (T_hat, xi_eff, I_AB, chi_BE) and the
recovery / sensitivity panels live in results/study/figures/ and are produced by
scripts/make_plots.py + scripts/make_extra_plots.py.
"""
import csv
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPORT2 = HERE
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
STUDY = os.path.join(ROOT, "results", "study")
FIGS = os.path.join(REPORT2, "figures")
os.makedirs(FIGS, exist_ok=True)

ATTACKS = ["saturation", "lo", "intercept_resend", "collective"]
LABELS = {
    "saturation": "Насыщение детектора",
    "lo": "Манипуляция LO",
    "intercept_resend": "Перехват-пересылка",
    "collective": "Коллективная атака",
}
COLOR = {
    "saturation": "#1f77b4", "lo": "#ff7f0e",
    "intercept_resend": "#2ca02c", "collective": "#d62728",
}


def load(path, xkey="distance_km"):
    with open(os.path.join(STUDY, path)) as f:
        rows = list(csv.DictReader(f))
    x = [float(r[xkey]) for r in rows]
    cols = {k: [float(r[k]) for r in rows] for k in rows[0] if k != xkey}
    return x, cols


def max_secure(dist, kbeta, thr=1e-4):
    ds = [d for d, k in zip(dist, kbeta) if k > thr]
    return max(ds) if ds else 0.0


# ---------------------------------------------------------------------------
# 6.1 — nominal regime
# ---------------------------------------------------------------------------
def fig_6_1():
    d, c = load("baseline.csv")
    # (a) security metrics
    plt.figure(figsize=(8, 5))
    plt.plot(d, c["I_AB"], "o-", color="#1f77b4", label=r"$I_{AB}$ (взаимная информация А$\leftrightarrow$Б)")
    plt.plot(d, c["chi_BE"], "s-", color="#ff7f0e", label=r"$\chi_{BE}$ (граница Холево, утечка к Еве)")
    plt.plot(d, c["K_asymp"], "v:", color="#9467bd", lw=1.5, label=r"$K_{асимп}=I_{AB}-\chi_{BE}$")
    plt.plot(d, c["K_beta"], "^-", color="#2ca02c", lw=2.4, label=r"$K_\beta=\beta I_{AB}-\chi_{BE}$ (секретный ключ)")
    plt.axhline(0, color="k", lw=0.6)
    plt.axvline(60, color="gray", ls="--", lw=1.0)
    plt.text(60.5, 0.9, "предел\nсекретности\n~60 км", fontsize=8, color="gray")
    plt.xlabel("Расстояние L, км")
    plt.ylabel("бит / символ")
    plt.title("Рисунок 6.1 — Номинальный режим: ключевые метрики vs расстояние\n"
              r"($\alpha=2{,}0$, $V_A=4$ SNU, $\eta=0{,}6$, $v_{el}=0{,}015$, $\xi=0{,}01$, $\beta=0{,}95$)")
    plt.legend(fontsize=9)
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(FIGS, "fig_6_1_baseline.png"), dpi=140)
    plt.close()

    # (b) channel estimation quality
    fig, ax = plt.subplots(1, 2, figsize=(11, 4.3))
    ax[0].plot(d, c["T_true"], "k-", lw=2.0, label=r"$T$ истинная $=10^{-0{,}02L}$")
    ax[0].plot(d, c["T_hat"], "o", color="#d62728", ms=6, label=r"$\hat{T}$ оценка (регрессия)")
    ax[0].set_xlabel("Расстояние L, км")
    ax[0].set_ylabel("Трансмиссивность")
    ax[0].set_title("(а) Оценка трансмиссивности канала")
    ax[0].legend(); ax[0].grid(alpha=0.3)

    ax[1].plot(d, c["xi_eff"], "s-", color="#ff7f0e", lw=2.0, label=r"$\xi_{eff}$ оценка")
    ax[1].axhline(0.01, color="k", ls="--", lw=1.0, label=r"истинное $\xi=0{,}01$ SNU")
    ax[1].set_xlabel("Расстояние L, км")
    ax[1].set_ylabel(r"$\xi_{eff}$, SNU")
    ax[1].set_title("(б) Оценка избыточного шума\n(рост на больших L — конечность выборки)")
    ax[1].legend(); ax[1].grid(alpha=0.3)
    fig.suptitle("Рисунок 6.2 — Номинальный режим: качество оценивания параметров канала", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.94))
    fig.savefig(os.path.join(FIGS, "fig_6_1_estimation.png"), dpi=140)
    plt.close(fig)


# ---------------------------------------------------------------------------
# 6.2 — influence of channel parameters
# ---------------------------------------------------------------------------
def fig_6_2():
    # family K_beta vs distance for several xi
    d, c = load("channel_kbeta_vs_dist_xi.csv")
    plt.figure(figsize=(8, 5))
    cmap = plt.get_cmap("viridis")
    keys = [k for k in c]
    for i, k in enumerate(keys):
        xi = k.replace("xi_", "")
        plt.plot(d, c[k], "o-", color=cmap(i / max(1, len(keys) - 1)),
                 label=rf"$\xi={xi}$ SNU")
    plt.axhline(0, color="k", lw=0.6)
    plt.xlabel("Расстояние L, км")
    plt.ylabel(r"$K_\beta$, бит / символ")
    plt.title("Рисунок 6.3 — Влияние избыточного шума $\\xi$ на секретную скорость\n"
              "(чем выше $\\xi$, тем ниже $K_\\beta$ и короче дальность)")
    plt.legend()
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(os.path.join(FIGS, "fig_6_2_family.png"), dpi=140)
    plt.close()

    # metrics vs xi at 25 km
    xi, c = load("channel_xi_at25km.csv", xkey="xi")
    fig, ax1 = plt.subplots(figsize=(8, 5))
    ax1.plot(xi, c["K_beta"], "^-", color="#2ca02c", lw=2.4, label=r"$K_\beta$")
    ax1.plot(xi, c["I_AB"], "o-", color="#1f77b4", label=r"$I_{AB}$")
    ax1.plot(xi, c["chi_BE"], "s-", color="#ff7f0e", label=r"$\chi_{BE}$")
    ax1.axhline(0, color="k", lw=0.6)
    # mark the security cutoff
    kb = c["K_beta"]
    cutoff = None
    for a, b, xa, xb in zip(kb, kb[1:], xi, xi[1:]):
        if a > 1e-4 >= b:
            cutoff = xa + (xb - xa) * (a - 1e-4) / (a - b)
            break
    if cutoff:
        ax1.axvline(cutoff, color="red", ls="--", lw=1.2)
        ax1.text(cutoff + 0.003, 0.3, f"$K_\\beta\\to 0$\nпри $\\xi\\approx{cutoff:.2f}$",
                 color="red", fontsize=9)
    ax1.set_xlabel(r"Избыточный шум $\xi$, SNU")
    ax1.set_ylabel("бит / символ")
    ax1.set_title("Рисунок 6.4 — Влияние избыточного шума $\\xi$ на метрики (L = 25 км)")
    ax1.legend()
    ax1.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(FIGS, "fig_6_2_xi.png"), dpi=140)
    plt.close(fig)


# ---------------------------------------------------------------------------
# 6.5 — baseline vs attacks (comparison)
# ---------------------------------------------------------------------------
def fig_6_5():
    bd, bc = load("baseline.csv")
    sel = [1.0, 25.0, 50.0]
    idx = [bd.index(s) for s in sel]
    base_vals = [bc["K_beta"][i] for i in idx]
    attack_vals = {}
    secure = {"baseline": max_secure(bd, bc["K_beta"])}
    for a in ATTACKS:
        d, c = load(f"attack_{a}.csv")
        attack_vals[a] = [c["K_beta"][d.index(s)] for s in sel]
        secure[a] = max_secure(d, c["K_beta"])

    fig, ax = plt.subplots(1, 2, figsize=(13, 5))
    # grouped bars
    n = 1 + len(ATTACKS)
    w = 0.8 / n
    x = np.arange(len(sel))
    ax[0].bar(x + 0 * w - 0.4 + w / 2, base_vals, w, color="black", label="Без атаки")
    for j, a in enumerate(ATTACKS, start=1):
        ax[0].bar(x + j * w - 0.4 + w / 2, attack_vals[a], w, color=COLOR[a], label=LABELS[a])
    ax[0].set_xticks(x)
    ax[0].set_xticklabels([f"{int(s)} км" for s in sel])
    ax[0].set_ylabel(r"$K_\beta$, бит / символ")
    ax[0].set_title("(а) Секретная скорость: без атаки vs атаки")
    ax[0].legend(fontsize=8)
    ax[0].grid(alpha=0.3, axis="y")

    # max secure distance
    names = ["baseline"] + ATTACKS
    labels = ["Без атаки"] + [LABELS[a] for a in ATTACKS]
    colors = ["black"] + [COLOR[a] for a in ATTACKS]
    vals = [secure[n_] for n_ in names]
    ax[1].barh(range(len(names)), vals, color=colors)
    ax[1].set_yticks(range(len(names)))
    ax[1].set_yticklabels(labels, fontsize=9)
    ax[1].invert_yaxis()
    ax[1].set_xlabel("Макс. дальность секретности, км")
    ax[1].set_title("(б) Граница дальности (где $K_\\beta>0$)")
    for i, v in enumerate(vals):
        ax[1].text(v + 0.5, i, f"{v:.0f}", va="center", fontsize=9)
    ax[1].grid(alpha=0.3, axis="x")
    fig.suptitle("Рисунок 6.12 — Сравнение характеристик системы в присутствии и отсутствии атак", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.95))
    fig.savefig(os.path.join(FIGS, "fig_6_5_compare.png"), dpi=140)
    plt.close(fig)


# ---------------------------------------------------------------------------
# 6.6 — attack vs recovery (comparison)
# ---------------------------------------------------------------------------
def fig_6_6():
    sel = [1.0, 25.0, 50.0]
    fig, ax = plt.subplots(1, 2, figsize=(13, 5))
    x = np.arange(len(ATTACKS))
    w = 0.38
    attack_at = {s: [] for s in sel}
    rec_at = {s: [] for s in sel}
    sec_atk, sec_rec = [], []
    for a in ATTACKS:
        ad, ac = load(f"attack_{a}.csv")
        rd, rc = load(f"recovered_{a}.csv")
        for s in sel:
            attack_at[s].append(ac["K_beta"][ad.index(s)])
            rec_at[s].append(rc["K_beta"][rd.index(s)])
        sec_atk.append(max_secure(ad, ac["K_beta"]))
        sec_rec.append(max_secure(rd, rc["K_beta"]))

    # K_beta @ 25 km: attack vs recovered
    s = 25.0
    ax[0].bar(x - w / 2, attack_at[s], w, color="#d62728", label="Под атакой")
    ax[0].bar(x + w / 2, rec_at[s], w, color="#2ca02c", label="После восстановления")
    ax[0].set_xticks(x)
    ax[0].set_xticklabels([LABELS[a] for a in ATTACKS], rotation=15, fontsize=8)
    ax[0].set_ylabel(r"$K_\beta$, бит / символ")
    ax[0].set_title("(а) Восстановление $K_\\beta$ при L = 25 км")
    ax[0].legend(); ax[0].grid(alpha=0.3, axis="y")

    # max secure distance: attack vs recovered
    ax[1].barh(x - w / 2, sec_atk, w, color="#d62728", label="Под атакой")
    ax[1].barh(x + w / 2, sec_rec, w, color="#2ca02c", label="После восстановления")
    ax[1].set_yticks(x)
    ax[1].set_yticklabels([LABELS[a] for a in ATTACKS], fontsize=8)
    ax[1].invert_yaxis()
    ax[1].set_xlabel("Макс. дальность секретности, км")
    ax[1].set_title("(б) Восстановление дальности")
    ax[1].axvline(60, color="gray", ls="--", lw=1.0)
    ax[1].text(60.5, -0.4, "baseline 60 км", color="gray", fontsize=8)
    ax[1].legend(); ax[1].grid(alpha=0.3, axis="x")
    fig.suptitle("Рисунок 6.13 — Сравнение характеристик системы при атаке и после восстановления", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.95))
    fig.savefig(os.path.join(FIGS, "fig_6_6_recovery.png"), dpi=140)
    plt.close(fig)


# ---------------------------------------------------------------------------
# 6.7 — applicability boundaries
# ---------------------------------------------------------------------------
def fig_6_7():
    bd, bc = load("baseline.csv")
    rows = [("Без атаки (baseline)", max_secure(bd, bc["K_beta"]), "black")]
    for a in ATTACKS:
        ad, ac = load(f"attack_{a}.csv")
        rd, rc = load(f"recovered_{a}.csv")
        rows.append((f"{LABELS[a]} — атака", max_secure(ad, ac["K_beta"]), COLOR[a]))
        rows.append((f"{LABELS[a]} — восстановл.", max_secure(rd, rc["K_beta"]), COLOR[a]))
    labels = [r[0] for r in rows]
    vals = [r[1] for r in rows]
    cols = [r[2] for r in rows]
    alphas = []
    for r in rows:
        alphas.append(0.5 if "атака" in r[0] else (1.0 if "восстан" in r[0] else 1.0))
    plt.figure(figsize=(9, 7))
    bars = plt.barh(range(len(rows)), vals, color=cols)
    for b, al in zip(bars, alphas):
        b.set_alpha(al)
    plt.yticks(range(len(rows)), labels, fontsize=9)
    plt.gca().invert_yaxis()
    plt.axvline(60, color="gray", ls="--", lw=1.0)
    plt.text(60.5, len(rows) - 1, "baseline\n60 км", color="gray", fontsize=8)
    for i, v in enumerate(vals):
        plt.text(v + 0.5, i, f"{v:.0f} км", va="center", fontsize=8)
    plt.xlabel("Макс. дальность сохранения секретности, км")
    plt.title("Рисунок 6.14 — Границы применимости протокола\n(дальность секретности по сценариям)")
    plt.grid(alpha=0.3, axis="x")
    plt.tight_layout()
    plt.savefig(os.path.join(FIGS, "fig_6_7_applicability.png"), dpi=140)
    plt.close()


if __name__ == "__main__":
    fig_6_1()
    fig_6_2()
    fig_6_5()
    fig_6_6()
    fig_6_7()
    print(f"Section-6 figures written to {FIGS}")
