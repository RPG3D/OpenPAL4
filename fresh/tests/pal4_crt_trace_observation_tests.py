"""Reject incomplete evidence and report the first unaligned load draw."""
import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_pal4_crt_traces import inspect


class CrtTraceTests(unittest.TestCase):
    def pair(self):
        left = [dict(kind=k, value=n, caller=0x5CCC25, source=None)
                for n, k in enumerate([0, 1, 0] * 5)]
        right = [dict(r, caller=0, source=r'H:\native\native_game_save_load.cpp') for r in left]
        return [dict(enabled=True, count=15, dropped=0, records=records) for records in (left, right)]

    def test_first_draw_difference_is_not_hidden_by_matching_later_calls(self):
        a, b = self.pair()
        self.assertTrue(inspect(a, b)['save_values_equal'])
        b['records'][0]['value'] = 900
        result = inspect(a, b)
        self.assertFalse(result['save_values_equal'])
        self.assertEqual(result['first_different_save_call'], 0)

    def test_partial_overflow_disabled_and_malformed_prefixes_are_rejected(self):
        a, b = self.pair()
        for field, value in [('count', 14), ('dropped', 1), ('enabled', False)]:
            damaged = copy.deepcopy(b); damaged[field] = value
            with self.assertRaises(ValueError):
                inspect(a, damaged)
        b['records'].pop(); b['count'] -= 1
        with self.assertRaises(ValueError):
            inspect(a, b)


if __name__ == '__main__':
    unittest.main()
