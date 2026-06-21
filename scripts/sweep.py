#!/usr/bin/env python3
"""
Sweep script: run the cvqkd simulator for a set of distances and a JSON base configuration,
collect xi_max and key_rate outputs into a CSV file.

Usage: python3 scripts/sweep.py <path-to-exe> <base-config.json> <output.csv>

The script expects the executable to accept: --config <file> --output <file>
and that the executable writes a JSON result or CSV; the script will attempt to parse stdout
if the executable prints JSON; otherwise it will collect output CSV files passed via --output.
"""
import sys
import subprocess
import json
import csv
import os

if len(sys.argv) < 4:
    print("Usage: sweep.py <exe> <base_config.json> <out.csv>")
    sys.exit(1)

exe = sys.argv[1]
base_cfg_path = sys.argv[2]
out_csv = sys.argv[3]

with open(base_cfg_path,'r',encoding='utf-8') as f:
    base = json.load(f)

distances = base.get('distances_km', [])
base_params = base.get('base', {})
name = base.get('name','sweep')

rows = []
for d in distances:
    cfg = dict(base)
    cfg['base'] = dict(base_params)
    cfg['base']['distance_km'] = d
    cfg['distances_km'] = [d]
    cfg_path = f"temp_{name}_{d}.json"
    out_path = f"result_{name}_{d}.csv"
    with open(cfg_path,'w',encoding='utf-8') as cf:
        json.dump(cfg, cf, indent=2)

    cmd = [exe, '--config', cfg_path, '--output', out_path]
    print('Running:', ' '.join(cmd))
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print(proc.stdout)
    except subprocess.CalledProcessError as e:
        print('Execution failed:', e.stderr)
        continue

    # try to read out_path as CSV and extract xi_max, key_rate
    if os.path.exists(out_path):
        with open(out_path,'r',encoding='utf-8') as oc:
            reader = csv.DictReader(oc)
            for r in reader:
                r['distance_km'] = d
                rows.append(r)
    else:
        # attempt to parse stdout as JSON
        try:
            data = json.loads(proc.stdout)
            # flatten relevant fields
            rows.append({
                'distance_km': d,
                'xi_max': data.get('xi_max'),
                'key_rate_per_symbol': data.get('key_rate_per_symbol')
            })
        except Exception:
            print('No result for distance', d)

# write aggregated CSV
if rows:
    keys = sorted(rows[0].keys())
    with open(out_csv,'w',encoding='utf-8',newline='') as f:
        writer = csv.DictWriter(f, fieldnames=keys)
        writer.writeheader()
        for r in rows:
            writer.writerow(r)
    print('Wrote', out_csv)
else:
    print('No rows collected')
