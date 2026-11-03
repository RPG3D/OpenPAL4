"""Regression for the live305 false joint-consumer divergence at MT475."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from analyze_pal4_combat_traces import is_joint, compare_normal_prepare


class PrepareCrtTests(unittest.TestCase):
    def documents(self):
        documents = []
        for native, values in ((False, [14802, 24752, 31827, 29088]),
                               (True, [28872, 7692, 23778, 3110])):
            records = [{"kind": 0, "value": value, "state_before": index,
                        "state_after": index + 1,
                        "caller": 0 if native else (0x53D00D, 0x53D2B8)[index % 2],
                        "source": "H:\\fresh\\native\\renderware\\combat_manager_events_native.cpp" if native else None}
                       for index, value in enumerate(values)]
            documents.append({"scope": "combat_prepare CRT calls only; not a complete RNG stream",
                              "trace": {"generator": "crt", "enabled": True, "count": 4,
                                        "dropped": 0, "records": records}})
        return documents

    def test_two_encounters_choose_different_openings_despite_background_match(self):
        result = compare_normal_prepare(*self.documents())
        self.assertFalse(result["all_choices_equal"])
        for row, choices in zip(result["encounters"], [("CA5", "CA3"), ("CA3", "CA5")]):
            self.assertEqual((row["original"]["opening"], row["native"]["opening"]), choices)
            self.assertEqual(row["original"]["background"], row["native"]["background"])

    def test_rejects_incomplete_unmapped_and_non_crt_evidence(self):
        for field, value in (("dropped", 1), ("enabled", False), ("count", 3), ("generator", "mt")):
            with self.subTest(field=field), self.assertRaises(ValueError):
                documents = self.documents()
                documents[0]["trace"][field] = value
                compare_normal_prepare(*documents)
        mutations = [lambda r: r[1].update(caller=0x53D00D),
                     lambda r: r[1].update(state_before=99),
                     lambda r: r[0].update(value=-1)]
        for mutate in mutations:
            with self.assertRaises(ValueError):
                documents = self.documents()
                mutate(documents[0]["trace"]["records"])
                compare_normal_prepare(*documents)
        documents = self.documents()
        documents[1]["trace"]["records"][0]["source"] = "other.cpp"
        with self.assertRaises(ValueError):
            compare_normal_prepare(*documents)


class JointConsumerTests(unittest.TestCase):
    def test_recorded_two_draw_joint_has_no_false_divergence(self):
        # Original live305 MT474/475 are the two JointAttack return sites;
        # MT476 is EndAttack. The old second address misclassified MT475.
        original = [0x558337, 0x558438, 0x56500C]
        native = ["NativeCombatJointAttack::Impl::RandomNumber"] * 2 + ["EndAttack"]
        self.assertEqual([is_joint({"caller": a}, True) for a in original],
                         [is_joint({"source": s}, False) for s in native])
        self.assertEqual([is_joint({"caller": a}, True) for a in original],
                         [True, True, False])

    def test_call_instruction_is_not_a_recorded_return_address(self):
        self.assertFalse(is_joint({"caller": 0x558433}, True))
        # Live301's real divergence at208 must continue to distinguish the
        # original EndAttack consumer from the native extra joint draw.
        self.assertFalse(is_joint({"caller": 0x56500C}, True))
        self.assertTrue(is_joint({"source": "NativeCombatJointAttack::Impl::RandomNumber"}, False))


if __name__ == "__main__":
    unittest.main()
