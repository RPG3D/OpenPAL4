"""Focused evidence must survive full-prefix overflow without hiding mismatches."""
import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_pal4_crt_focused import inspect, active_attribution, check_counters


class FocusedTests(unittest.TestCase):
    def test_current_interval_without_script_preserves_counts_and_seed(self):
        a = self.document()
        a['script']['records'] = []; a['script']['count'] = 0
        a['seeds']['records'][0]['thread'] = 7
        a['attribution'] = dict(dropped=0, intervals=[], current=dict(
            seed=dict(sequence=899990, value=123, thread=7), calls=3, unclassified=0,
            counters=[dict(caller=0x42ab63, thread=7, source_line=0, source=None, count=3)]))
        self.assertEqual(active_attribution(a, True)['calls'], 3)
        self.assertEqual(inspect(a,a,True)['observations']['original']['current_attribution']['calls'],3)
        for key,value in [('seed',None),('calls',4),('unclassified',1)]:
            b=copy.deepcopy(a);b['attribution']['current'][key]=value
            with self.assertRaises(ValueError): active_attribution(b,True)
        b=copy.deepcopy(a);b['attribution']['current']['counters']*=2
        with self.assertRaises(ValueError): active_attribution(b,True)
        with self.assertRaises(ValueError): active_attribution(self.document(),True)

    def test_ordered_prefix_is_bounded_and_keeps_original_ordinals(self):
        counter = dict(caller=0x42ab63, thread=7, source_line=0, source=None, count=3)
        row = dict(seed=dict(sequence=90, value=123, thread=7), calls=3,
                   unclassified=0, counters=[counter], prefix_capacity=2,
                   prefix_dropped=1, prefix=[dict(counter, full_sequence=91+i, value=100+i)
                                             for i in range(2)])
        check_counters(row)
        for field, value in [('prefix_dropped', 0), ('prefix_capacity', 0), ('prefix', [])]:
            invalid = copy.deepcopy(row); invalid[field] = value
            with self.assertRaises(ValueError): check_counters(invalid)
        for field in ('prefix', 'prefix_capacity', 'prefix_dropped'):
            invalid = copy.deepcopy(row); del invalid[field]
            with self.assertRaises(ValueError): check_counters(invalid)
        for field, value in [('full_sequence', 95), ('caller', 123), ('value', -1)]:
            invalid = copy.deepcopy(row); invalid['prefix'][0][field] = value
            with self.assertRaises(ValueError): check_counters(invalid)
        invalid = copy.deepcopy(row); invalid['counters'][0]['count'] = 1
        invalid['counters'].append(dict(counter, caller=999, count=2))
        with self.assertRaises(ValueError): check_counters(invalid)
        self.assertEqual(row['prefix'][0]['value'], 100)

    def document(self):
        stream = lambda records: dict(enabled=True, count=len(records), dropped=0, records=records)
        return dict(script=stream([dict(kind=0, value=19, full_sequence=900001)]),
                    seeds=stream([dict(kind=1, value=123, full_sequence=899990)]))

    def test_distinct_values_and_seed_distance_are_retained(self):
        a = self.document()
        b = copy.deepcopy(a)
        b['script']['records'][0]['value'] = 16
        result = inspect(a, b)
        self.assertFalse(result['script_values_equal'])
        self.assertEqual(result['first_different_script_call'], 0)
        self.assertEqual(result['observations']['original']['script'][0]['intervening_observed_calls'], 10)
        self.assertEqual(a['script']['records'][0]['value'], 19)

    def test_missing_overflow_and_invalid_ordinals_do_not_pass(self):
        a = self.document()
        for name in ['script', 'seeds']:
            for field, value in [('enabled', False), ('dropped', 1), ('count', 7)]:
                b = copy.deepcopy(a)
                b[name][field] = value
                with self.assertRaises(ValueError):
                    inspect(a, b)
        b = self.document()
        b['seeds']['records'][0]['full_sequence'] = 900001
        with self.assertRaises(ValueError):
            inspect(a, b)
        b = self.document()
        b['script']['records'].clear()
        b['script']['count'] = 0
        self.assertFalse(inspect(a, b)['script_values_equal'])

    def test_attribution_conservation_and_identity(self):
        a = self.document()
        for stream in ['script', 'seeds']:
            a[stream]['records'][0]['thread'] = 7
        a['attribution'] = dict(dropped=0, intervals=[dict(
            script_sequence=900001, script_value=19, script_thread=7,
            seed=dict(sequence=899990, value=123, thread=7),
            calls=10, unclassified=0, counters=[dict(
                caller=0x646b63, thread=7, source_line=0, source=None, count=10)])])
        self.assertTrue(inspect(a, a, True)['script_values_equal'])
        for key, value in [('calls', 9), ('unclassified', 1), ('script_value', 18), ('seed', None)]:
            b = copy.deepcopy(a)
            b['attribution']['intervals'][0][key] = value
            with self.assertRaises(ValueError):
                inspect(a, b, True)
        b = copy.deepcopy(a)
        b['attribution']['intervals'][0]['counters'][0]['count'] = 9
        with self.assertRaises(ValueError):
            inspect(a, b, True)
        with self.assertRaises(ValueError):
            inspect(self.document(), a, True)


if __name__ == '__main__':
    unittest.main()
