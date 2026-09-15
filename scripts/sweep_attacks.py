#!/usr/bin/env python3
"""
Sweep по JSON-конфигам: для каждой атаки создаёт отдельный конфиг
и запускает симулятор. Результаты собираются в общий CSV.
"""
import sys
import os
import json
import subprocess
import csv
from pathlib import Path

def run_simulation(exe, config_path, output_path):
    """Запуск симулятора с одним конфигом."""
    cmd = [exe, "--config", config_path, "--out", output_path]
    print(f"  Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  ERROR: {result.stderr}")
        return False
    return True

def merge_csvs(csv_files, output_path):
    """Объединить несколько CSV файлов в один."""
    if not csv_files:
        return
    
    headers = None
    rows = []
    for f in csv_files:
        with open(f, 'r', encoding='utf-8') as fp:
            reader = csv.DictReader(fp)
            if headers is None:
                headers = reader.fieldnames
            for row in reader:
                rows.append(row)
    
    with open(output_path, 'w', newline='', encoding='utf-8') as fp:
        writer = csv.DictWriter(fp, fieldnames=headers)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Merged {len(rows)} rows into {output_path}")

def main():
    if len(sys.argv) < 4:
        print("Usage: sweep_attacks.py <exe> <config.json> <output.csv>")
        print("  The config.json should have 'attacks' as a list of attack names.")
        sys.exit(1)
    
    exe = sys.argv[1]
    config_path = sys.argv[2]
    output_path = sys.argv[3]
    
    # Загрузить базовый конфиг
    with open(config_path, 'r', encoding='utf-8') as f:
        base_config = json.load(f)
    
    attacks = base_config.get('attacks', ['none'])
    
    # Для каждой атаки создаём отдельный конфиг и запускаем
    temp_configs = []
    temp_csvs = []
    
    for i, attack in enumerate(attacks):
        # Модифицируем конфиг для одной атаки
        config = base_config.copy()
        config['attacks'] = [attack]
        config['name'] = f"{base_config['name']}_{attack}"
        
        temp_config = f"_temp_config_{i}.json"
        temp_csv = f"_temp_result_{i}.csv"
        
        with open(temp_config, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2)
        
        print(f"\n[{i+1}/{len(attacks)}] Attack: {attack}")
        success = run_simulation(exe, temp_config, temp_csv)
        
        if success:
            temp_configs.append(temp_config)
            temp_csvs.append(temp_csv)
        
        # Удаляем временный конфиг
        os.remove(temp_config)
    
    # Объединяем результаты
    if temp_csvs:
        merge_csvs(temp_csvs, output_path)
        for f in temp_csvs:
            os.remove(f)
    
    print(f"\nDone! Results saved to: {output_path}")

if __name__ == '__main__':
    main()