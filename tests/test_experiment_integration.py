"""CLI regression tests; no third-party Python packages are required."""

import argparse
import copy
import csv
import json
import math
import subprocess
import unittest
from pathlib import Path


class ExperimentIntegration(unittest.TestCase):
    executable: Path
    workdir: Path

    def setUp(self):
        self.directory = self.workdir / self.id().rsplit(".", 1)[-1]
        self.directory.mkdir(parents=True, exist_ok=True)
        self.config = {
            "name": "integration",
            "base": {
                "alpha": 0.5, "T": 0.6, "xi": 0.02, "eta": 0.6,
                "v_el": 0.05, "saturation_level": None, "N": 40000, "seed": 1942,
            },
            "attacks": [], "runs_per_point": 1,
        }

    def execute(self, config, tag="run", success=True):
        config_path = self.directory / f"{tag}.json"
        output_path = self.directory / f"{tag}.csv"
        config_path.write_text(json.dumps(config, ensure_ascii=False), encoding="utf-8")
        result = subprocess.run(
            [str(self.executable), "--config", str(config_path), "--out", str(output_path)],
            capture_output=True, text=True, encoding="utf-8", check=False,
        )
        if not success:
            self.assertNotEqual(result.returncode, 0, result.stdout)
            return result.stderr
        self.assertEqual(result.returncode, 0, result.stderr)
        with output_path.open(newline="", encoding="utf-8") as stream:
            rows = list(csv.DictReader(stream))
        self.assertTrue(rows)
        for row in rows:
            self.assertNotIn(None, row)
            row["details"] = json.loads(row["details"])
            row["attack_params"] = json.loads(row["attack_params"])
        return rows

    @staticmethod
    def deterministic_rows(rows):
        return [{key: value for key, value in row.items() if key != "elapsed_ms"} for row in rows]

    def test_repeats_distances_and_seed_reproducibility(self):
        self.config["runs_per_point"] = 3
        self.config["distances_km"] = [0, 10]
        self.config["attacks"] = [{"name": "collective", "coupling": 0.3, "excess_noise": 0.05}]
        first = self.execute(self.config)
        second = self.execute(self.config, "repeat")
        self.assertEqual(len(first), 6)
        self.assertEqual(self.deterministic_rows(first), self.deterministic_rows(second))
        self.assertEqual(len({row["details"]["seed"] for row in first}), 6)
        self.assertEqual([float(row["T_true"]) for row in first[:3]], [1.0] * 3)
        self.assertAlmostEqual(float(first[3]["T_true"]), 10 ** -0.2)
        self.assertNotEqual(first[0]["xi_hat"], first[1]["xi_hat"])

    def test_csv_roundtrips_unicode_quotes_newlines_and_nested_json(self):
        self.config["name"] = 'ИР, "контроль"\nвторая строка'
        metadata = {"source": {"title": 'A, "quoted"\nreference', "parameters": [1, 2]}}
        self.config["attacks"] = [{"name": "collective", "coupling": 0, "excess_noise": 0,
                                   "metadata": metadata}]
        row = self.execute(self.config)[0]
        self.assertEqual(row["scenario"], self.config["name"])
        self.assertEqual(row["attack_params"][0]["metadata"], metadata)
        self.assertEqual(len(row["details"]["source_revision"]), 40)
        self.assertEqual(len(row["details"]["source_fingerprint"]), 64)

    def test_explicit_transmission_and_zero_attack_identity(self):
        baseline = self.execute(self.config)[0]
        self.assertEqual(float(baseline["T_true"]), 0.6)
        for attack in [
            {"name": "collective", "coupling": 0, "excess_noise": 0},
            {"name": "intercept_resend", "eve_mode": "ideal", "fraction": 0},
        ]:
            config = copy.deepcopy(self.config)
            config["attacks"] = [attack]
            row = self.execute(config, attack["name"])[0]
            for key in ["T_hat", "xi_hat", "I_AB", "chi_BE", "K_beta"]:
                self.assertEqual(row[key], baseline[key])
            if attack["name"] == "intercept_resend":
                self.assertEqual(row["details"]["Alice_Eve"]["erasures"], self.config["base"]["N"])

    def test_estimated_parameters_reach_security_without_double_counting(self):
        self.config["base"]["N"] = 200000
        self.config["attacks"] = [{"name": "collective", "coupling": 0.6, "excess_noise": 0.3}]
        row = self.execute(self.config)[0]
        details = row["details"]
        predicted = details["prediction"]
        self.assertAlmostEqual(predicted["T"], 0.6 * (1 - 0.6**2))
        self.assertAlmostEqual(predicted["xi"], 0.02 + 0.3 / 0.6)
        reference = details["gaussian_reference"]
        self.assertAlmostEqual(reference["T_used"], float(row["T_hat"]))
        self.assertAlmostEqual(reference["xi_used"], max(0, float(row["xi_hat"])))
        observed = details["moments"]
        self.assertAlmostEqual(details["security"]["T_used"],
                               observed["c2"] ** 2 / (0.5**4 * 0.6))
        self.assertEqual(details["security"]["I_AB"], details["Alice_Bob"]["I_discrete"])
        self.assertFalse(details["security"]["finite_key_certified"])
        self.assertNotAlmostEqual(reference["T_used"], 0.6, places=2)
        self.assertAlmostEqual(float(row["xi_hat"]), predicted["xi"], delta=0.12)

    def test_full_physical_ir_zero_key_and_real_eve_calibration(self):
        for mode in ["ideal", "real"]:
            config = copy.deepcopy(self.config)
            config["base"]["N"] = 200000
            config["attacks"] = [{"name": "intercept_resend", "eve_mode": mode}]
            row = self.execute(config, mode)[0]
            details = row["details"]
            self.assertEqual(details["Alice_Eve"]["observed"], 200000)
            self.assertGreater(details["Alice_Eve"]["I_discrete"], details["Alice_Bob"]["I_discrete"])
            self.assertEqual(float(row["K_beta"]), 0)
            addition = 2.0 if mode == "ideal" else 2 * 1.05 / 0.6
            self.assertAlmostEqual(details["prediction"]["xi"], 0.02 + addition)
            self.assertAlmostEqual(float(row["xi_hat"]), 0.02 + addition, delta=0.15)
            self.assertEqual(row["attack_params"][0]["stage"], "source")
            if mode == "real":
                self.assertEqual(row["attack_params"][0]["meas_eff"], config["base"]["eta"])
                self.assertEqual(row["attack_params"][0]["v_el"], config["base"]["v_el"])

    def test_oracle_and_saturation_do_not_certify_security(self):
        for mode, saturation in [("oracle", None), ("real", 0.1)]:
            config = copy.deepcopy(self.config)
            config["base"]["saturation_level"] = saturation
            config["attacks"] = [{"name": "intercept_resend", "eve_mode": mode}]
            row = self.execute(config, mode)[0]
            details = row["details"]
            self.assertFalse(details["security"]["model_supported"])
            self.assertFalse(details["security"]["finite_key_certified"])
            self.assertTrue(math.isnan(float(row["K_beta"])))
            if mode == "oracle":
                self.assertEqual(details["Alice_Eve"]["SER"], 0)
                self.assertEqual(details["Alice_Eve"]["BER_gray"], 0)
            else:
                self.assertGreater(details["Eve_clipped_components"], 0)
                self.assertGreater(details["Bob_clipped_components"], 0)

    def test_partial_interception_erasures_and_gaussian_interval_warning(self):
        self.config["attacks"] = [{"name": "intercept_resend", "eve_mode": "ideal", "fraction": 0.3}]
        details = self.execute(self.config)[0]["details"]
        self.assertAlmostEqual(details["Alice_Eve"]["observed"] / details["N"], 0.3, delta=0.015)
        self.assertEqual(details["Alice_Eve"]["total"], details["N"])
        self.assertFalse(details["estimate"]["gaussian_interval_assumptions"])

    def test_zero_transmission_unidentifiable(self):
        self.config["base"]["T"] = 0
        details = self.execute(self.config)[0]["details"]
        self.assertFalse(details["estimate"]["identifiable"])
        self.assertIsNone(details["estimate"]["xi_upper"])
        self.assertIsNone(details["prediction"]["xi"])
        self.assertNotIn("gaussian_reference", details)

    def test_unknown_and_invalid_parameters_fail(self):
        cases = [
            {"attacks": ["not_an_attack"]}, {"attacks": [{"name": "collective", "excess_nois": 0.05}]},
            {"attacks": [{"name": "intercept_resend", "eve_mode": "ideal", "meas_eff": 0.6}]},
            {"attacks": [{"name": "collective", "coupling": 1.01}]},
            {"runs_per_point": 0}, {"runs_per_point": -1}, {"runs_per_point": 1.5},
            {"distances_km": [-1]}, {"base": {"N": -1}}, {"base": {"N": 1}},
            {"base": {"eta": 0}}, {"base": {"xi": -0.1}},
            {"base": {"T": 1.01}}, {"base": {"eps_total": 1e-9}},
        ]
        for index, patch in enumerate(cases):
            with self.subTest(patch=patch):
                config = copy.deepcopy(self.config)
                if "base" in patch:
                    config["base"].update(patch["base"])
                else:
                    config.update(patch)
                self.execute(config, f"invalid_{index}", success=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--workdir", type=Path, required=True)
    args = parser.parse_args()
    ExperimentIntegration.executable = args.exe.resolve()
    ExperimentIntegration.workdir = args.workdir.resolve()
    unittest.main(argv=["test_experiment_integration"], verbosity=2)
