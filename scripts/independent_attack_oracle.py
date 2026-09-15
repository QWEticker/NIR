#!/usr/bin/env python3
"""Independent analytical and Monte Carlo oracle for article validation."""

import argparse
import csv
import hashlib
import json
import math
import platform
import random
import statistics
import sys
from functools import lru_cache
from pathlib import Path


def _continued_beta_fraction(a, b, x):
    maximum_iterations = 300
    epsilon = 3.0e-14
    tiny = 1.0e-300
    qab = a + b
    qap = a + 1.0
    qam = a - 1.0
    c = 1.0
    d = 1.0 - qab * x / qap
    if abs(d) < tiny:
        d = tiny
    d = 1.0 / d
    result = d
    for iteration in range(1, maximum_iterations + 1):
        twice = 2 * iteration
        numerator = iteration * (b - iteration) * x / (
            (qam + twice) * (a + twice)
        )
        d = 1.0 + numerator * d
        c = 1.0 + numerator / c
        if abs(d) < tiny:
            d = tiny
        if abs(c) < tiny:
            c = tiny
        d = 1.0 / d
        result *= d * c
        numerator = -(a + iteration) * (qab + iteration) * x / (
            (a + twice) * (qap + twice)
        )
        d = 1.0 + numerator * d
        c = 1.0 + numerator / c
        if abs(d) < tiny:
            d = tiny
        if abs(c) < tiny:
            c = tiny
        d = 1.0 / d
        change = d * c
        result *= change
        if abs(change - 1.0) <= epsilon:
            return result
    raise ArithmeticError("incomplete beta fraction did not converge")


def regularized_beta(x, a, b):
    if not 0.0 <= x <= 1.0 or a <= 0.0 or b <= 0.0:
        raise ValueError("invalid beta arguments")
    if x == 0.0:
        return 0.0
    if x == 1.0:
        return 1.0
    log_factor = (
        math.lgamma(a + b)
        - math.lgamma(a)
        - math.lgamma(b)
        + a * math.log(x)
        + b * math.log1p(-x)
    )
    factor = math.exp(log_factor)
    if x < (a + 1.0) / (a + b + 2.0):
        return factor * _continued_beta_fraction(a, b, x) / a
    return 1.0 - factor * _continued_beta_fraction(b, a, 1.0 - x) / b


def beta_quantile(probability, a, b):
    if not 0.0 <= probability <= 1.0:
        raise ValueError("probability must be in [0, 1]")
    if probability == 0.0:
        return 0.0
    if probability == 1.0:
        return 1.0
    lower, upper = 0.0, 1.0
    for _ in range(100):
        midpoint = (lower + upper) / 2.0
        if regularized_beta(midpoint, a, b) < probability:
            lower = midpoint
        else:
            upper = midpoint
    return (lower + upper) / 2.0


def _regularized_gamma_p(a, x):
    if a <= 0.0 or x < 0.0:
        raise ValueError("invalid gamma arguments")
    if x == 0.0:
        return 0.0
    epsilon = 3.0e-14
    tiny = 1.0e-300
    log_scale = -x + a * math.log(x) - math.lgamma(a)
    if x < a + 1.0:
        term = 1.0 / a
        total = term
        ap = a
        for _ in range(100000):
            ap += 1.0
            term *= x / ap
            total += term
            if abs(term) <= abs(total) * epsilon:
                return total * math.exp(log_scale)
        raise ArithmeticError("incomplete gamma series did not converge")
    b = x + 1.0 - a
    c = 1.0 / tiny
    d = 1.0 / b
    fraction = d
    for iteration in range(1, 100000):
        coefficient = -iteration * (iteration - a)
        b += 2.0
        d = coefficient * d + b
        if abs(d) < tiny:
            d = tiny
        c = b + coefficient / c
        if abs(c) < tiny:
            c = tiny
        d = 1.0 / d
        change = d * c
        fraction *= change
        if abs(change - 1.0) <= epsilon:
            return 1.0 - math.exp(log_scale) * fraction
    raise ArithmeticError("incomplete gamma fraction did not converge")


@lru_cache(maxsize=None)
def chi_square_quantile(probability, degrees_of_freedom):
    if not 0.0 < probability < 1.0 or degrees_of_freedom <= 0.0:
        raise ValueError("invalid chi-square arguments")
    lower = 0.0
    upper = max(
        1.0,
        degrees_of_freedom
        + 12.0 * math.sqrt(2.0 * degrees_of_freedom)
        + 24.0,
    )
    while _regularized_gamma_p(degrees_of_freedom / 2.0, upper / 2.0) < probability:
        upper *= 2.0
    for _ in range(100):
        midpoint = (lower + upper) / 2.0
        value = _regularized_gamma_p(degrees_of_freedom / 2.0, midpoint / 2.0)
        if value < probability:
            lower = midpoint
        else:
            upper = midpoint
    return (lower + upper) / 2.0


@lru_cache(maxsize=None)
def student_quantile(probability, degrees_of_freedom):
    if not 0.0 < probability < 1.0 or degrees_of_freedom <= 0.0:
        raise ValueError("invalid Student arguments")
    if probability == 0.5:
        return 0.0
    if probability < 0.5:
        return -student_quantile(1.0 - probability, degrees_of_freedom)
    if degrees_of_freedom > 200.0:
        z = statistics.NormalDist().inv_cdf(probability)
        inverse = 1.0 / degrees_of_freedom
        return (
            z
            + (z**3 + z) * inverse / 4.0
            + (5.0 * z**5 + 16.0 * z**3 + 3.0 * z) * inverse**2 / 96.0
            + (
                3.0 * z**7
                + 19.0 * z**5
                + 17.0 * z**3
                - 15.0 * z
            )
            * inverse**3
            / 384.0
        )
    lower, upper = 0.0, 1.0
    while True:
        x = degrees_of_freedom / (degrees_of_freedom + upper * upper)
        cdf = 1.0 - 0.5 * regularized_beta(
            x, degrees_of_freedom / 2.0, 0.5
        )
        if cdf >= probability:
            break
        upper *= 2.0
    for _ in range(100):
        midpoint = (lower + upper) / 2.0
        x = degrees_of_freedom / (degrees_of_freedom + midpoint * midpoint)
        cdf = 1.0 - 0.5 * regularized_beta(
            x, degrees_of_freedom / 2.0, 0.5
        )
        if cdf < probability:
            lower = midpoint
        else:
            upper = midpoint
    return (lower + upper) / 2.0


def merged_case(document, case):
    parameters = dict(document["base"])
    parameters.update(case.get("base", {}))
    parameters.update(case)
    return parameters


def eve_calibration(parameters):
    mode = parameters.get("eve_mode", "physical")
    if mode == "real":
        return parameters["eta"], parameters["v_el"], False
    if mode == "ideal":
        return 1.0, 0.0, False
    if mode == "oracle":
        return 1.0, 0.0, True
    return (
        parameters.get("meas_eff", 0.8),
        parameters.get("eve_v_el", 0.0),
        False,
    )


def analytical_prediction(parameters):
    attack = parameters["attack"]
    transmission = parameters["T"]
    channel_noise = parameters["xi"]
    result = {
        "case_id": parameters["id"],
        "attack": attack,
        "T_expected": None,
        "xi_expected": None,
        "mean_gain": None,
        "power_factor": None,
        "gaussian_interval_assumptions": True,
        "analytical_supported": True,
    }
    if attack == "intercept_resend":
        fraction = parameters.get("fraction", 1.0)
        resend_gain = parameters.get("resend_gain", 1.0)
        mean_gain = 1.0 + fraction * (resend_gain - 1.0)
        gain_squared = mean_gain * mean_gain
        eta_eve, eve_noise, oracle = eve_calibration(parameters)
        measurement_noise = 0.0
        if not oracle:
            measurement_noise = 2.0 * (1.0 + eve_noise) / eta_eve
        gain_variance = (
            fraction
            * (1.0 - fraction)
            * (resend_gain - 1.0) ** 2
        )
        variance_alice = 2.0 * parameters["alpha"] ** 2
        result["mean_gain"] = mean_gain
        result["power_factor"] = gain_squared
        result["T_expected"] = transmission * gain_squared
        if transmission > 0.0 and gain_squared > 0.0:
            numerator = (
                channel_noise
                + fraction * resend_gain**2 * measurement_noise
                + variance_alice * gain_variance
            )
            result["xi_expected"] = numerator / gain_squared
        result["gaussian_interval_assumptions"] = fraction in (0.0, 1.0)
        if transmission == 0.0 or gain_squared == 0.0:
            result["gaussian_interval_assumptions"] = False
        eve_limit = parameters.get("eve_saturation", math.inf)
        if eve_limit is not None and math.isfinite(eve_limit):
            result["analytical_supported"] = False
            result["gaussian_interval_assumptions"] = False
    elif attack == "collective":
        coupling = parameters.get("coupling", 0.3)
        power_factor = 1.0 - coupling * coupling
        result["power_factor"] = power_factor
        result["T_expected"] = transmission * power_factor
        if transmission > 0.0:
            result["xi_expected"] = (
                channel_noise
                + parameters.get("excess_noise", 0.05) / transmission
            )
        else:
            result["gaussian_interval_assumptions"] = False
    else:
        raise ValueError(f"unsupported attack: {attack}")
    receiver_limit = parameters.get("saturation_level", math.inf)
    if receiver_limit is not None and math.isfinite(receiver_limit):
        result["analytical_supported"] = False
        result["gaussian_interval_assumptions"] = False
    return result


def _clamp(value, limit):
    if math.isfinite(limit):
        return min(max(value, -limit), limit)
    return value


def simulate_run(parameters, samples, seed, confidence_level):
    rng = random.Random(seed)
    alpha = parameters["alpha"]
    transmission = parameters["T"]
    channel_noise = parameters["xi"]
    receiver_eta = parameters["eta"]
    receiver_noise = parameters["v_el"]
    receiver_limit = parameters.get("saturation_level", math.inf)
    if receiver_limit is None:
        receiver_limit = math.inf
    sqrt_transmission = math.sqrt(transmission)
    channel_sigma = math.sqrt(transmission * channel_noise / 4.0)
    receiver_gain = math.sqrt(receiver_eta)
    receiver_sigma = math.sqrt((1.0 + receiver_noise) / 2.0)
    attack = parameters["attack"]
    if attack == "intercept_resend":
        fraction = parameters.get("fraction", 1.0)
        resend_gain = parameters.get("resend_gain", 1.0)
        eta_eve, eve_noise, oracle = eve_calibration(parameters)
        eve_gain = math.sqrt(eta_eve)
        eve_sigma = math.sqrt((1.0 + eve_noise) / 2.0)
        eve_limit = parameters.get("eve_saturation", math.inf)
        if eve_limit is None:
            eve_limit = math.inf
    else:
        coupling = parameters.get("coupling", 0.3)
        collective_scale = math.sqrt(1.0 - coupling * coupling)
        collective_sigma = math.sqrt(
            (1.0 - coupling * coupling)
            * parameters.get("excess_noise", 0.05)
            / 4.0
        )
    xx = 0.0
    xy = 0.0
    yy = 0.0
    phases = ((alpha, 0.0), (0.0, alpha), (-alpha, 0.0), (0.0, -alpha))
    for _ in range(samples):
        source_x, source_p = phases[rng.randrange(4)]
        components = [source_x, source_p]
        if attack == "intercept_resend" and rng.random() < fraction:
            if oracle:
                components = [resend_gain * value for value in components]
            else:
                measured = []
                for value in components:
                    detected = eve_gain * value + rng.gauss(0.0, eve_sigma)
                    measured.append(
                        resend_gain * _clamp(detected, eve_limit) / eve_gain
                    )
                components = measured
        for source_value, attacked_value in zip((source_x, source_p), components):
            optical = (
                sqrt_transmission * attacked_value
                + rng.gauss(0.0, channel_sigma)
            )
            if attack == "collective":
                optical = (
                    collective_scale * optical
                    + rng.gauss(0.0, collective_sigma)
                )
            observed = _clamp(
                receiver_gain * optical + rng.gauss(0.0, receiver_sigma),
                receiver_limit,
            )
            xx += source_value * source_value
            xy += source_value * observed
            yy += observed * observed
    count = 2 * samples
    slope = xy / xx
    residual_sum = max(0.0, yy - xy * xy / xx)
    degrees_of_freedom = count - 1
    residual_variance = residual_sum / degrees_of_freedom
    slope_se = math.sqrt(residual_variance / xx)
    half = (
        student_quantile(
            (1.0 + confidence_level) / 2.0, degrees_of_freedom
        )
        * slope_se
    )
    slope_lower = slope - half
    slope_upper = slope + half
    transmission_hat = slope * slope / receiver_eta
    if slope_lower <= 0.0 <= slope_upper:
        minimum_square = 0.0
    else:
        minimum_square = min(slope_lower**2, slope_upper**2)
    transmission_lower = minimum_square / receiver_eta
    transmission_upper = max(slope_lower**2, slope_upper**2) / receiver_eta
    detector_variance = (1.0 + receiver_noise) / 2.0
    xi_hat = math.nan
    if slope != 0.0:
        xi_hat = 4.0 * (residual_variance - detector_variance) / slope**2
    tail = (1.0 - confidence_level) / 4.0
    joint_half = (
        student_quantile(1.0 - tail, degrees_of_freedom) * slope_se
    )
    joint_slope_lower = slope - joint_half
    joint_slope_upper = slope + joint_half
    variance_lower = residual_sum / chi_square_quantile(
        1.0 - tail, degrees_of_freedom
    )
    variance_upper = residual_sum / chi_square_quantile(
        tail, degrees_of_freedom
    )
    identifiable = joint_slope_lower > 0.0
    if joint_slope_lower <= 0.0 <= joint_slope_upper:
        xi_lower = -math.inf
        xi_upper = math.inf
    else:
        candidates = [
            4.0 * (variance - detector_variance) / joint_slope**2
            for variance in (variance_lower, variance_upper)
            for joint_slope in (joint_slope_lower, joint_slope_upper)
        ]
        xi_lower = min(candidates)
        xi_upper = max(candidates)
    return {
        "seed": seed,
        "slope": slope,
        "T_hat": transmission_hat,
        "xi_hat": xi_hat,
        "T_lower": transmission_lower,
        "T_upper": transmission_upper,
        "xi_lower": xi_lower,
        "xi_upper": xi_upper,
        "identifiable": identifiable,
    }


def clopper_pearson(successes, trials, confidence_level):
    alpha = 1.0 - confidence_level
    lower = 0.0
    upper = 1.0
    if successes > 0:
        lower = beta_quantile(
            alpha / 2.0, successes, trials - successes + 1
        )
    if successes < trials:
        upper = beta_quantile(
            1.0 - alpha / 2.0, successes + 1, trials - successes
        )
    return lower, upper


def _sample_metrics(values, expected):
    finite = [value for value in values if math.isfinite(value)]
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
    if expected is None:
        bias = None
        rmse = None
    else:
        bias = mean - expected
        rmse = math.sqrt(
            statistics.fmean((value - expected) ** 2 for value in finite)
        )
    return {"mean": mean, "bias": bias, "sd": sd, "se": se, "rmse": rmse}


def summarize_runs(case_id, prediction, runs, samples):
    transmission = _sample_metrics(
        [run["T_hat"] for run in runs], prediction["T_expected"]
    )
    noise = _sample_metrics(
        [run["xi_hat"] for run in runs], prediction["xi_expected"]
    )
    coverage = None
    coverage_lower = None
    coverage_upper = None
    successes = 0
    if (
        prediction["gaussian_interval_assumptions"]
        and prediction["T_expected"] is not None
        and prediction["xi_expected"] is not None
    ):
        for run in runs:
            contains = (
                run["T_lower"]
                <= prediction["T_expected"]
                <= run["T_upper"]
                and run["xi_lower"]
                <= prediction["xi_expected"]
                <= run["xi_upper"]
            )
            run["joint_contains"] = contains
            successes += int(contains)
        coverage = successes / len(runs)
        coverage_lower, coverage_upper = clopper_pearson(
            successes, len(runs), 0.95
        )
    else:
        for run in runs:
            run["joint_contains"] = None
    bias_pass = True
    for metrics, absolute_floor in ((transmission, 0.01), (noise, 0.05)):
        if metrics["bias"] is not None:
            tolerance = max(absolute_floor, 4.0 * metrics["se"])
            bias_pass = bias_pass and abs(metrics["bias"]) <= tolerance
    coverage_pass = (
        coverage is None
        or coverage_lower <= 0.95 <= coverage_upper
    )
    return {
        "case_id": case_id,
        "attack": prediction["attack"],
        "samples": samples,
        "repeats": len(runs),
        "T_expected": prediction["T_expected"],
        "xi_expected": prediction["xi_expected"],
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
        "joint_coverage": coverage,
        "coverage_successes": successes if coverage is not None else None,
        "coverage_cp_lower": coverage_lower,
        "coverage_cp_upper": coverage_upper,
        "gaussian_interval_assumptions": prediction[
            "gaussian_interval_assumptions"
        ],
        "criteria_pass": bias_pass and coverage_pass,
    }


def write_csv(path, rows, fieldnames):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cases", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--analytical-only", action="store_true")
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
    confidence_level = campaign["confidence_level"]
    if samples < 3 or repeats < 1:
        raise ValueError("samples must be >= 3 and repeats must be positive")
    cases = [
        case
        for case in document["cases"]
        if not args.publication_only or case.get("publication", False)
    ]
    args.out_dir.mkdir(parents=True, exist_ok=True)
    predictions = []
    resolved = []
    for case in cases:
        parameters = merged_case(document, case)
        resolved.append(parameters)
        predictions.append(analytical_prediction(parameters))
    reference_fields = [
        "case_id",
        "attack",
        "T_expected",
        "xi_expected",
        "mean_gain",
        "power_factor",
        "gaussian_interval_assumptions",
        "analytical_supported",
    ]
    reference_path = args.out_dir / "reference.csv"
    write_csv(reference_path, predictions, reference_fields)
    output_paths = [reference_path]
    if not args.analytical_only:
        run_rows = []
        summaries = []
        for case_index, (parameters, prediction) in enumerate(
            zip(resolved, predictions)
        ):
            runs = []
            for run_index in range(repeats):
                run_seed = base_seed + case_index * 1_000_003 + run_index
                run = simulate_run(
                    parameters, samples, run_seed, confidence_level
                )
                run["case_id"] = parameters["id"]
                run["run_index"] = run_index
                runs.append(run)
            summaries.append(
                summarize_runs(parameters["id"], prediction, runs, samples)
            )
            run_rows.extend(runs)
        run_fields = [
            "case_id",
            "run_index",
            "seed",
            "slope",
            "T_hat",
            "xi_hat",
            "T_lower",
            "T_upper",
            "xi_lower",
            "xi_upper",
            "identifiable",
            "joint_contains",
        ]
        summary_fields = [
            "case_id",
            "attack",
            "samples",
            "repeats",
            "T_expected",
            "xi_expected",
            "T_mean",
            "T_bias",
            "T_sd",
            "T_se",
            "T_rmse",
            "xi_mean",
            "xi_bias",
            "xi_sd",
            "xi_se",
            "xi_rmse",
            "joint_coverage",
            "coverage_successes",
            "coverage_cp_lower",
            "coverage_cp_upper",
            "gaussian_interval_assumptions",
            "criteria_pass",
        ]
        runs_path = args.out_dir / "independent_runs.csv"
        summary_path = args.out_dir / "independent_summary.csv"
        write_csv(runs_path, run_rows, run_fields)
        write_csv(summary_path, summaries, summary_fields)
        output_paths.extend([runs_path, summary_path])
    manifest = {
        "schema_version": 1,
        "oracle": "independent_attack_oracle.py",
        "python": sys.version,
        "platform": platform.platform(),
        "cases_sha256": file_sha256(args.cases),
        "oracle_sha256": file_sha256(Path(__file__)),
        "samples": samples,
        "repeats": repeats,
        "base_seed": base_seed,
        "confidence_level": confidence_level,
        "publication_only": args.publication_only,
        "outputs": {
            path.name: file_sha256(path)
            for path in output_paths
        },
    }
    (args.out_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
