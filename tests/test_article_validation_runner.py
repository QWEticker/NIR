"""Tests for simulator campaign configuration and result comparison."""

import importlib.util
import json
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUNNER_PATH = ROOT / "scripts" / "run_article_validation.py"
sys.path.insert(0, str(RUNNER_PATH.parent))
SPEC = importlib.util.spec_from_file_location("run_article_validation", RUNNER_PATH)
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


class ArticleValidationRunner(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.document = json.loads(
            (ROOT / "validation" / "article_cases.json").read_text(encoding="utf-8")
        )
        cls.cases = {
            case["id"]: RUNNER.merged_case(cls.document, case)
            for case in cls.document["cases"]
        }

    def test_real_eve_configuration_uses_named_mode(self):
        config = RUNNER.simulator_config(
            self.cases["ir_real_full"], 1000, 3, 42
        )
        attack = config["attacks"][0]
        self.assertEqual(attack["eve_mode"], "real")
        self.assertNotIn("meas_eff", attack)
        self.assertNotIn("v_el", attack)
        self.assertEqual(config["runs_per_point"], 3)
        self.assertEqual(config["base"]["N"], 1000)

    def test_physical_eve_configuration_maps_calibration(self):
        config = RUNNER.simulator_config(
            self.cases["ir_gain_anchor"], 1000, 2, 43
        )
        attack = config["attacks"][0]
        self.assertEqual(attack["eve_mode"], "physical")
        self.assertEqual(attack["meas_eff"], 0.6)
        self.assertEqual(attack["v_el"], 0.05)
        self.assertEqual(attack["resend_gain"], 0.8)

    def test_collective_configuration_preserves_anchor(self):
        config = RUNNER.simulator_config(
            self.cases["collective_anchor"], 1000, 2, 44
        )
        attack = config["attacks"][0]
        self.assertEqual(attack["coupling"], 0.3)
        self.assertEqual(attack["excess_noise"], 0.05)
        self.assertEqual(
            attack["metadata"]["validation_case"], "collective_anchor"
        )

    def test_summary_includes_alice_bob_and_eve_statistics(self):
        details = {
            "N": 1000,
            "Alice_Bob": {"I_discrete": 0.2, "BER_gray": 0.3, "SER": 0.4},
            "Alice_Eve": {
                "I_discrete": 0.5,
                "BER_gray": 0.1,
                "SER": 0.2,
                "observed": 800,
                "total": 1000,
            },
            "source_revision": "a" * 40,
            "source_fingerprint": "b" * 64,
        }
        rows = [
            {"T_hat": "0.6", "xi_hat": "0.01", "details": json.dumps(details)},
            {"T_hat": "0.62", "xi_hat": "0.03", "details": json.dumps(details)},
        ]
        reference = {
            "attack": "intercept_resend",
            "T_expected": "0.61",
            "xi_expected": "0.02",
            "gaussian_interval_assumptions": "false",
        }
        summary = RUNNER.summarize("case", rows, reference)
        self.assertAlmostEqual(summary["I_AB_mean"], 0.2)
        self.assertAlmostEqual(summary["I_AE_mean"], 0.5)
        self.assertAlmostEqual(summary["BER_AE_mean"], 0.1)
        self.assertAlmostEqual(summary["Eve_observed_fraction"], 0.8)

    def test_summary_uses_marginal_interval_coverage(self):
        rows = []
        for index in range(20):
            details = {
                "N": 1000,
                "Alice_Bob": {
                    "I_discrete": 0.2,
                    "BER_gray": 0.3,
                    "SER": 0.4,
                },
                "estimate": {
                    "T_lower": 0.61 if index == 0 else 0.59,
                    "T_upper": 0.62 if index == 0 else 0.61,
                    "xi_lower": 0.02 if index == 1 else 0.0,
                    "xi_upper": 0.03 if index == 1 else 0.02,
                },
                "source_revision": "a" * 40,
                "source_fingerprint": "b" * 64,
            }
            rows.append(
                {
                    "T_hat": "0.6",
                    "xi_hat": "0.01",
                    "details": json.dumps(details),
                }
            )
        reference = {
            "attack": "collective",
            "T_expected": "0.6",
            "xi_expected": "0.01",
            "gaussian_interval_assumptions": "true",
        }
        summary = RUNNER.summarize("coverage", rows, reference)
        self.assertEqual(summary["T_coverage"], 0.95)
        self.assertEqual(summary["xi_coverage"], 0.95)
        self.assertEqual(summary["joint_coverage"], 0.9)
        self.assertTrue(summary["criteria_pass"])

        for row in rows:
            details = json.loads(row["details"])
            details["estimate"]["T_lower"] = 0.61
            details["estimate"]["T_upper"] = 0.62
            row["details"] = json.dumps(details)
        summary = RUNNER.summarize("undercoverage", rows, reference)
        self.assertFalse(summary["criteria_pass"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
