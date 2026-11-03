import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from pal4_particle_state_comparison import compare_fields


class ParticleStateComparisonTests(unittest.TestCase):
    def fixture(self):
        return dict(active=1, allocated=2, config_words={"276": 123},
                    particles=[dict(active=1, age_bits=10, life_bits=20),
                               dict(active=0, age_bits=30, life_bits=40)])

    def test_inactive_history_is_retained_separately(self):
        a = self.fixture()
        b = copy.deepcopy(a)
        b["particles"][1].update(age_bits=50, life_bits=60)
        result = compare_fields(a, b)
        self.assertEqual(result["differences"], [])
        self.assertEqual(result["inactive_slot_differences"][0]["index"], 1)
        self.assertEqual(b["particles"][1]["life_bits"], 60)

    def test_live_age_or_lifetime_difference_blocks(self):
        for field in ("age_bits", "life_bits"):
            a = self.fixture()
            b = copy.deepcopy(a)
            b["particles"][0][field] += 1
            self.assertIn("particles", compare_fields(a, b)["differences"])

    def test_activity_or_slot_count_difference_blocks(self):
        a = self.fixture()
        b = copy.deepcopy(a)
        b["particles"][1]["active"] = 1
        self.assertIn("particles", compare_fields(a, b)["differences"])
        b = copy.deepcopy(a)
        b["particles"].pop()
        self.assertIn("particles", compare_fields(a, b)["differences"])

    def test_configuration_counts_and_unreviewed_fields_still_block(self):
        a = self.fixture()
        for field in ("active", "allocated"):
            b = copy.deepcopy(a)
            b[field] += 1
            self.assertIn(field, compare_fields(a, b)["differences"])
        b = copy.deepcopy(a)
        b["config_words"]["276"] += 1
        self.assertIn("config_words", compare_fields(a, b)["differences"])
        b = copy.deepcopy(a)
        b["particles"][1]["unreviewed"] = 9
        self.assertIn("particles", compare_fields(a, b)["differences"])


if __name__ == "__main__":
    unittest.main()
