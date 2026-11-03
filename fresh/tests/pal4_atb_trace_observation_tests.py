import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from compare_pal4_atb_traces import compare


def trace():
    sample = dict(state=2, mode=0, active=0, random_index=4, count=2, error=0, actors=[
        dict(id=2501, group=2, progress_bits=1, turn=0, ready=1, action=0),
        dict(id=2501, group=2, progress_bits=2, turn=0, ready=1, action=0)])
    return dict(enabled=True, count=1, dropped=0, aborted=0, nested=0, pending=False,
                records=[dict(sequence=0, delta_bits=0x3c8b4396, aborted=False,
                              before=sample, after=copy.deepcopy(sample))])


class ObservationTests(unittest.TestCase):
    def test_distinguishes_clock_input_from_state_difference(self):
        a, b = trace(), trace()
        b['records'][0]['delta_bits'] += 1
        value = compare(a, b)
        self.assertIsNotNone(value['first_delta_difference'])
        self.assertIsNone(value['first_sample_differences']['before'])

    def test_preserves_duplicate_monster_order(self):
        a, b = trace(), trace()
        b['records'][0]['after']['actors'].reverse()
        self.assertIsNotNone(compare(a, b)['first_sample_differences']['after'])

    def test_rejects_incomplete_and_unknown_rng(self):
        for field in ['dropped', 'aborted', 'nested', 'pending']:
            a = trace()
            a[field] = 1
            with self.assertRaises(ValueError):
                compare(a, trace())
        a = trace()
        a['records'][0]['before']['random_index'] = 0xffffffff
        with self.assertRaises(ValueError):
            compare(a, trace())


if __name__ == '__main__':
    unittest.main()
