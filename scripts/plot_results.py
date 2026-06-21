#!/usr/bin/env python3
# Small script to visualize CSV results (requires pandas, matplotlib)
import sys
import pandas as pd
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print("Usage: plot_results.py results.csv")
    sys.exit(1)

df = pd.read_csv(sys.argv[1])
if 'distance_km' in df.columns and ('K' in df.columns or 'K_bit_per_sym' in df.columns):
    Kcol = 'K' if 'K' in df.columns else 'K_bit_per_sym'
    plt.figure()
    plt.plot(df['distance_km'], df[Kcol], marker='o')
    plt.xlabel('Distance (km)')
    plt.ylabel('K (bit/sym)')
    plt.grid(True)
    plt.savefig('K_vs_distance.png')
    print("Saved K_vs_distance.png")
else:
    print("CSV doesn't contain expected columns 'distance_km' and 'K' (or 'K_bit_per_sym'). Columns:", df.columns.tolist())
