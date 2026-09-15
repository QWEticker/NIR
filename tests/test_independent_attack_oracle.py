"""Tests for the standalone analytical and Monte Carlo validation oracle."""

import importlib.util
import json
import math
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ORACLE_PATH = ROOT / "scripts" / "independent_attack_oracle.py"
SPEC = importlib.util.spec_from_file_location("independent_attack_oracle", ORACLE_PATH)
ORACLE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ORACLE)


class IndependentAttackOracle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.document = json.loads(
            (ROOT / "validation" / "article_cases.json").read_text(encoding="utf-8")
        )
        cls.cases = {
            case["id"]: ORACLE.merged_case(cls.document, case)
            for case in cls.document["cases"]
        }

    def test_ir_anchor_matches_closed_form(self):
        prediction = ORACLE.analytical_prediction(self.cases["ir_gain_anchor"])
        self.assertAlmostEqual(prediction["T_expected"], 0.384)
        self.assertAlmostEqual(prediction["xi_expected"], 3.515625)

    def test_ideal_and_real_eve_have_expected_noise(self):
        ideal = ORACLE.analytical_prediction(self.cases["ir_ideal_full"])
        real = ORACLE.analytical_prediction(self.cases["ir_real_full"])
        self.assertAlmostEqual(ideal["xi_expected"], 2.01)
        self.assertAlmostEqual(real["xi_expected"], 3.51)

    def test_collective_anchor_matches_closed_form(self):
        prediction = ORACLE.analytical_prediction(self.cases["collective_anchor"])
        self.assertAlmostEqual(prediction["T_expected"], 0.546)
        self.assertAlmostEqual(
            prediction["xi_expected"], 0.09333333333333334
        )

    def test_nonidentifiable_cases_leave_noise_undefined(self):
        zero_resend = ORACLE.analytical_prediction(self.cases["ir_zero_resend"])
        zero_transmission = ORACLE.analytical_prediction(
            self.cases["collective_zero_transmission"]
        )
        self.assertEqual(zero_resend["T_expected"], 0.0)
        self.assertIsNone(zero_resend["xi_expected"])
        self.assertEqual(zero_transmission["T_expected"], 0.0)
        self.assertIsNone(zero_transmission["xi_expected"])

    def test_partial_ir_disables_gaussian_coverage_claim(self):
        prediction = ORACLE.analytical_prediction(self.cases["ir_partial_gain"])
        self.assertFalse(prediction["gaussian_interval_assumptions"])

    def test_reference_quantiles(self):
        self.assertAlmostEqual(
            ORACLE.student_quantile(0.975, 19), 2.093024054408263, places=10
        )
        self.assertAlmostEqual(
            ORACLE.chi_square_quantile(0.025, 19),
            8.906516481987971,
            places=9,
        )
        self.assertAlmostEqual(
            ORACLE.chi_square_quantile(0.975, 19),
            32.85232686172969,
            places=9,
        )

    def test_monte_carlo_is_seed_reproducible(self):
        parameters = self.cases["collective_anchor"]
        first = ORACLE.simulate_run(parameters, 5000, 481, 0.95)
        second = ORACLE.simulate_run(parameters, 5000, 481, 0.95)
        self.assertEqual(first, second)
        prediction = ORACLE.analytical_prediction(parameters)
        self.assertAlmostEqual(
            first["T_hat"], prediction["T_expected"], delta=0.08
        )
        self.assertTrue(math.isfinite(first["xi_hat"]))

    def test_coverage_criterion_uses_marginal_intervals(self):
        runs = []
        for index in range(20):
            runs.append(
                {
                    "T_hat": 0.6,
                    "xi_hat": 0.01,
                    "T_lower": 0.61 if index == 0 else 0.59,
                    "T_upper": 0.62 if index == 0 else 0.61,
                    "xi_lower": 0.02 if index == 1 else 0.0,
                    "xi_upper": 0.03 if index == 1 else 0.02,
                }
            )
        prediction = {
            "attack": "collective",
            "T_expected": 0.6,
            "xi_expected": 0.01,
            "gaussian_interval_assumptions": True,
        }
        summary = ORACLE.summarize_runs("coverage", prediction, runs, 1000)
        self.assertEqual(summary["T_coverage"], 0.95)
        self.assertEqual(summary["xi_coverage"], 0.95)
        self.assertEqual(summary["joint_coverage"], 0.9)
        self.assertTrue(summary["criteria_pass"])

        for run in runs:
            run["T_lower"] = 0.61
            run["T_upper"] = 0.62
        summary = ORACLE.summarize_runs("undercoverage", prediction, runs, 1000)
        self.assertFalse(summary["criteria_pass"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
