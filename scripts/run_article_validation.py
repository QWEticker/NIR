#!/usr/bin/env python3
"""Run simulator and independent-oracle campaigns for the attack articles."""

import argparse
import csv
import hashlib
import json
import math
import statistics
import subprocess
import sys
from pathlib import Path

SCRIPT_DIRECTORY = Path(__file__).resolve().parent
if str(SCRIPT_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIRECTORY))

from independent_attack_oracle import clopper_pearson


def merged_case(document, case):
    parameters = dict(document["base"])
    parameters.update(case.get("base", {}))
    parameters.update(case)
    return parameters


def simulator_config(parameters, samples, repeats, seed):
    base_keys = (
        "alpha",
        "T",
        "xi",
        "eta",
        "v_el",
        "beta",
        "saturation_level",
    )
    base = {key: parameters[key] for key in base_keys}
    base.update({"N": samples, "seed": seed})
    attack = {
        "name": parameters["attack"],
        "metadata": {"validation_case": parameters["id"]},
    }
    if parameters["attack"] == "intercept_resend":
        mode = parameters.get("eve_mode", "physical")
        attack.update(
            {
                "eve_mode": mode,
                "fraction": parameters.get("fraction", 1.0),
                "resend_gain": parameters.get("resend_gain", 1.0),
            }
        )
        if mode == "physical":
            attack["meas_eff"] = parameters.get("meas_eff", 0.8)
            attack["v_el"] = parameters.get("eve_v_el", 0.0)
            if "eve_saturation" in parameters:
                attack["saturation_level"] = parameters["eve_saturation"]
    elif parameters["attack"] == "collective":
        attack.update(
            {
                "coupling": parameters.get("coupling", 0.3),
                "excess_noise": parameters.get("excess_noise", 0.05),
            }
        )
    else:
        raise ValueError(f"unsupported attack: {parameters['attack']}")
    return {
        "name": parameters["id"],
        "base": base,
        "attacks": [attack],
        "runs_per_point": repeats,
    }


def read_csv(path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path, rows, fieldnames):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def finite_float(value):
    if value in (None, ""):
        return None
    number = float(value)
    return number if math.isfinite(number) else None


def sample_metrics(values, expected):
    finite = [value for value in values if value is not None]
    if not finite:
        return {
            "mean": None,
            "bias": None,
            "sd": None,
            "se": None,
            "rmse": None,
        }
    mean = statistics.fmean(finite)
    sd = statistics.stdev(finite) if len(finite) > 1 else 0.0
    se = sd / math.sqrt(len(finite))
    bias = None if expected is None else mean - expected
    rmse = None
    if expected is not None:
        rmse = math.sqrt(
            statistics.fmean((value - expected) ** 2 for value in finite)
        )
    return {"mean": mean, "bias": bias, "sd": sd, "se": se, "rmse": rmse}


def beta_interval(successes, trials, confidence_level):
    return clopper_pearson(successes, trials, confidence_level)


def summarize(case_id, rows, reference):
    run_details = [json.loads(row["details"]) for row in rows]
    expected_transmission = finite_float(reference["T_expected"])
    expected_noise = finite_float(reference["xi_expected"])
    transmission = sample_metrics(
        [finite_float(row["T_hat"]) for row in rows], expected_transmission
    )
    noise = sample_metrics(
        [finite_float(row["xi_hat"]) for row in rows], expected_noise
    )
    alice_bob = {
        metric: sample_metrics(
            [finite_float(details["Alice_Bob"][metric]) for details in run_details],
            None,
        )
        for metric in ("I_discrete", "BER_gray", "SER")
    }
    alice_eve = {
        metric: sample_metrics(
            [
                finite_float(details.get("Alice_Eve", {}).get(metric))
                for details in run_details
            ],
            None,
        )
        for metric in ("I_discrete", "BER_gray", "SER")
    }
    eve_observed_fraction = sample_metrics(
        [
            finite_float(details.get("Alice_Eve", {}).get("observed"))
            / finite_float(details.get("Alice_Eve", {}).get("total"))
            if finite_float(details.get("Alice_Eve", {}).get("total"))
            else None
            for details in run_details
        ],
        None,
    )
    gaussian = reference["gaussian_interval_assumptions"].lower() == "true"
    transmission_coverage = None
    transmission_successes = None
    transmission_coverage_lower = None
    transmission_coverage_upper = None
    noise_coverage = None
    noise_successes = None
    noise_coverage_lower = None
    noise_coverage_upper = None
    joint_coverage = None
    if gaussian and expected_transmission is not None and expected_noise is not None:
        transmission_successes = 0
        noise_successes = 0
        joint_successes = 0
        for details in run_details:
            estimate = details["estimate"]
            transmission_contains = (
                estimate["T_lower"]
                <= expected_transmission
                <= estimate["T_upper"]
            )
            noise_contains = (
                estimate["xi_lower"] <= expected_noise <= estimate["xi_upper"]
            )
            transmission_successes += int(transmission_contains)
            noise_successes += int(noise_contains)
            joint_successes += int(transmission_contains and noise_contains)
        transmission_coverage = transmission_successes / len(rows)
        noise_coverage = noise_successes / len(rows)
        joint_coverage = joint_successes / len(rows)
        (
            transmission_coverage_lower,
            transmission_coverage_upper,
        ) = beta_interval(transmission_successes, len(rows), 0.95)
        noise_coverage_lower, noise_coverage_upper = beta_interval(
            noise_successes, len(rows), 0.95
        )
    bias_pass = True
    for metrics, floor in ((transmission, 0.01), (noise, 0.05)):
        if metrics["bias"] is not None:
            bias_pass = bias_pass and abs(metrics["bias"]) <= max(
                floor, 4.0 * metrics["se"]
            )
    coverage_pass = transmission_coverage is None or (
        transmission_coverage_lower <= 0.95 <= transmission_coverage_upper
        and noise_coverage_lower <= 0.95 <= noise_coverage_upper
    )
    details = run_details[0]
    return {
        "case_id": case_id,
        "attack": reference["attack"],
        "samples": details["N"],
        "repeats": len(rows),
        "T_expected": expected_transmission,
        "xi_expected": expected_noise,
        "T_mean": transmission["mean"],
        "T_bias": transmission["bias"],
        "T_sd": transmission["sd"],
        "T_se": transmission["se"],
        "T_rmse": transmission["rmse"],
        "xi_mean": noise["mean"],
        "xi_bias": noise["bias"],
        "xi_sd": noise["sd"],
        "xi_se": noise["se"],
        "xi_rmse": noise["rmse"],
        "I_AB_mean": alice_bob["I_discrete"]["mean"],
        "I_AB_sd": alice_bob["I_discrete"]["sd"],
        "BER_AB_mean": alice_bob["BER_gray"]["mean"],
        "SER_AB_mean": alice_bob["SER"]["mean"],
        "I_AE_mean": alice_eve["I_discrete"]["mean"],
        "I_AE_sd": alice_eve["I_discrete"]["sd"],
        "BER_AE_mean": alice_eve["BER_gray"]["mean"],
        "SER_AE_mean": alice_eve["SER"]["mean"],
        "Eve_observed_fraction": eve_observed_fraction["mean"],
        "T_coverage": transmission_coverage,
        "T_coverage_successes": transmission_successes,
        "T_coverage_cp_lower": transmission_coverage_lower,
        "T_coverage_cp_upper": transmission_coverage_upper,
        "xi_coverage": noise_coverage,
        "xi_coverage_successes": noise_successes,
        "xi_coverage_cp_lower": noise_coverage_lower,
        "xi_coverage_cp_upper": noise_coverage_upper,
        "joint_coverage": joint_coverage,
        "gaussian_interval_assumptions": gaussian,
        "criteria_pass": bias_pass and coverage_pass,
        "source_revision": details["source_revision"],
        "source_fingerprint": details["source_fingerprint"],
    }


def comparison_rows(simulator, oracle):
    oracle_by_id = {row["case_id"]: row for row in oracle}
    comparisons = []
    for row in simulator:
        other = oracle_by_id[row["case_id"]]
        comparison = {
            "case_id": row["case_id"],
            "attack": row["attack"],
            "T_expected": row["T_expected"],
            "xi_expected": row["xi_expected"],
            "T_simulator_mean": row["T_mean"],
            "T_oracle_mean": finite_float(other["T_mean"]),
            "xi_simulator_mean": row["xi_mean"],
            "xi_oracle_mean": finite_float(other["xi_mean"]),
            "T_mean_difference": None,
            "xi_mean_difference": None,
            "simulator_criteria_pass": row["criteria_pass"],
            "oracle_criteria_pass": other["criteria_pass"].lower() == "true",
            "comparison_pass": True,
        }
        for prefix, floor in (("T", 0.01), ("xi", 0.05)):
            simulator_mean = comparison[f"{prefix}_simulator_mean"]
            oracle_mean = comparison[f"{prefix}_oracle_mean"]
            if simulator_mean is None or oracle_mean is None:
                continue
            difference = simulator_mean - oracle_mean
            comparison[f"{prefix}_mean_difference"] = difference
            simulator_se = row[f"{prefix}_se"]
            oracle_se = finite_float(other[f"{prefix}_se"])
            tolerance = max(
                floor,
                4.0 * math.sqrt(simulator_se**2 + oracle_se**2),
            )
            comparison["comparison_pass"] = (
                comparison["comparison_pass"]
                and abs(difference) <= tolerance
            )
        comparison["comparison_pass"] = (
            comparison["comparison_pass"]
            and comparison["simulator_criteria_pass"]
            and comparison["oracle_criteria_pass"]
        )
        comparisons.append(comparison)
    return comparisons


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--cases", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--publication-only", action="store_true")
    parser.add_argument("--samples", type=int)
    parser.add_argument("--repeats", type=int)
    parser.add_argument("--seed", type=int)
    args = parser.parse_args()
    document = json.loads(args.cases.read_text(encoding="utf-8"))
    campaign = document["campaign"]
    samples = args.samples or campaign["samples"]
    repeats = args.repeats or campaign["repeats"]
    base_seed = args.seed if args.seed is not None else campaign["seed"]
    if samples < 3 or repeats < 1:
        raise ValueError("samples must be >= 3 and repeats must be positive")
    cases = [
        case
        for case in document["cases"]
        if not args.publication_only or case.get("publication", False)
    ]
    args.out_dir.mkdir(parents=True, exist_ok=True)
    oracle_dir = args.out_dir / "oracle"
    simulator_dir = args.out_dir / "simulator"
    config_dir = simulator_dir / "configs"
    per_case_dir = simulator_dir / "per_case"
    config_dir.mkdir(parents=True, exist_ok=True)
    per_case_dir.mkdir(parents=True, exist_ok=True)
    oracle_command = [
        sys.executable,
        str(Path(__file__).with_name("independent_attack_oracle.py")),
        "--cases",
        str(args.cases),
        "--out-dir",
        str(oracle_dir),
        "--samples",
        str(samples),
        "--repeats",
        str(repeats),
        "--seed",
        str(base_seed),
    ]
    if args.publication_only:
        oracle_command.append("--publication-only")
    subprocess.run(oracle_command, check=True)
    reference_rows = read_csv(oracle_dir / "reference.csv")
    references = {row["case_id"]: row for row in reference_rows}
    raw_rows = []
    simulator_summaries = []
    simulator_artifacts = []
    for case_index, case in enumerate(cases):
        parameters = merged_case(document, case)
        case_seed = base_seed + case_index * 1_000_003
        config = simulator_config(parameters, samples, repeats, case_seed)
        config_path = config_dir / f"{parameters['id']}.json"
        output_path = per_case_dir / f"{parameters['id']}.csv"
        config_path.write_text(
            json.dumps(config, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        subprocess.run(
            [
                str(args.exe.resolve()),
                "--config",
                str(config_path),
                "--out",
                str(output_path),
            ],
            check=True,
        )
        simulator_artifacts.extend((config_path, output_path))
        rows = read_csv(output_path)
        raw_rows.extend(rows)
        simulator_summaries.append(
            summarize(parameters["id"], rows, references[parameters["id"]])
        )
    raw_path = simulator_dir / "simulator_runs.csv"
    summary_path = simulator_dir / "simulator_summary.csv"
    write_csv(raw_path, raw_rows, raw_rows[0].keys())
    summary_fields = list(simulator_summaries[0].keys())
    write_csv(summary_path, simulator_summaries, summary_fields)
    oracle_summary = read_csv(oracle_dir / "independent_summary.csv")
    comparisons = comparison_rows(simulator_summaries, oracle_summary)
    comparison_path = args.out_dir / "comparison.csv"
    write_csv(comparison_path, comparisons, comparisons[0].keys())
    revisions = sorted({row["source_revision"] for row in simulator_summaries})
    fingerprints = sorted(
        {row["source_fingerprint"] for row in simulator_summaries}
    )
    outputs = [
        oracle_dir / "reference.csv",
        oracle_dir / "independent_runs.csv",
        oracle_dir / "independent_summary.csv",
        oracle_dir / "manifest.json",
        *simulator_artifacts,
        raw_path,
        summary_path,
        comparison_path,
    ]
    manifest = {
        "schema_version": 1,
        "samples": samples,
        "repeats": repeats,
        "base_seed": base_seed,
        "publication_only": args.publication_only,
        "cases_sha256": file_sha256(args.cases),
        "runner_sha256": file_sha256(Path(__file__)),
        "oracle_sha256": file_sha256(
            Path(__file__).with_name("independent_attack_oracle.py")
        ),
        "executable_sha256": file_sha256(args.exe),
        "source_revisions": revisions,
        "source_fingerprints": fingerprints,
        "outputs": {
            str(path.relative_to(args.out_dir)): file_sha256(path)
            for path in outputs
        },
        "all_comparisons_pass": all(
            row["comparison_pass"] for row in comparisons
        ),
    }
    (args.out_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
