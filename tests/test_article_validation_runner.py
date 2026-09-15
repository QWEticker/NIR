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


if __name__ == "__main__":
    unittest.main(verbosity=2)
