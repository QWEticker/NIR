"""Физические иллюстрации для отчёта: созвездие QPSK и секторы фазовой плоскости."""
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Wedge

plt.rcParams.update({"font.size": 12, "font.family": "DejaVu Sans"})
OUT = __import__("os").path.join(__import__("os").path.dirname(__file__), "figures")

# --- fig_3_1: созвездие QPSK ---
alpha = 2.0
ks = [0, 1, 2, 3]
pts = [alpha * (1j ** k) for k in ks]
fig, ax = plt.subplots(figsize=(6.0, 6.0))
ax.axhline(0, color="0.7", lw=1); ax.axvline(0, color="0.7", lw=1)
rng = np.random.default_rng(7)
for k, p in zip(ks, pts):
    cloud = p + (rng.standard_normal(400) + 1j * rng.standard_normal(400)) * 0.28
    ax.scatter(cloud.real, cloud.imag, s=6, alpha=0.25, color="C0")
    ax.scatter([p.real], [p.imag], s=120, color="C3", zorder=5, edgecolor="k")
    ax.annotate(f"k={k}\n"+r"$\alpha i^{%d}$" % k, (p.real, p.imag),
                textcoords="offset points", xytext=(12, 12), fontsize=11)
ax.set_xlabel("X (амплитудная квадратура)")
ax.set_ylabel("P (фазовая квадратура)")
ax.set_title(r"Созвездие QPSK: $a=\alpha\, i^{k}$,  $V_A=\alpha^2$", fontsize=12)
lim = alpha * 1.8
ax.set_xlim(-lim, lim); ax.set_ylim(-lim, lim); ax.set_aspect("equal")
fig.tight_layout(); fig.savefig(f"{OUT}/fig_3_1_qpsk_constellation.png", dpi=160, bbox_inches="tight")
plt.close(fig)

# --- fig_3_4: секторы фазовой плоскости (дискретизация по углу) ---
fig, ax = plt.subplots(figsize=(5.2, 5.2))
colors = ["#cfe8ff", "#d7f0d0", "#ffe2cc", "#f3d4ef"]
labels = ["сектор 0\n[-45°,45°)", "сектор 1\n[45°,135°)",
          "сектор 2\n[135°,225°)", "сектор 3\n[225°,315°)"]
R = 3.0
for i in range(4):
    th0 = -45 + 90 * i
    ax.add_patch(Wedge((0, 0), R, th0, th0 + 90, facecolor=colors[i],
                        edgecolor="0.4", lw=1.2, alpha=0.85))
for i in range(4):
    a = np.deg2rad(90 * i)
    ax.scatter([2.0 * np.cos(a)], [2.0 * np.sin(a)], s=130, color="C3",
               zorder=6, edgecolor="k")
    ax.annotate(f"k̂={i}", (2.0*np.cos(a), 2.0*np.sin(a)),
                textcoords="offset points", xytext=(8, 8), fontsize=12)
for i in range(4):
    a = np.deg2rad(-45 + 90 * i)
    ax.plot([0, R*np.cos(a)], [0, R*np.sin(a)], "k--", lw=1)
# пример измерения
ym = 1.1 + 0.7j
ax.scatter([ym.real], [ym.imag], s=110, marker="*", color="k", zorder=7)
ax.annotate("(X,P)", (ym.real, ym.imag), textcoords="offset points",
            xytext=(8, 4), fontsize=10)
ax.set_xlabel(r"X      $\hat k=\lfloor(\varphi+\pi/4)/(\pi/2)\rfloor\ \mathrm{mod}\ 4$")
ax.set_xlim(-R, R); ax.set_ylim(-R, R); ax.set_aspect("equal")
ax.axhline(0, color="0.7", lw=0.8); ax.axvline(0, color="0.7", lw=0.8)
ax.set_ylabel("P")
ax.set_title("Дискретизация по фазовому углу: 4 сектора", fontsize=12)
fig.tight_layout(); fig.savefig(f"{OUT}/fig_3_4_phase_sectors.png", dpi=160, bbox_inches="tight")
plt.close(fig)
print("physics figures written")
