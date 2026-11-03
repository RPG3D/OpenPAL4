"""Diagnostic tests for unstable read rejection and full-state comparison."""
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from pal4_random_state import (INDEX_ADDRESS, STATE_ADDRESS, CRT_STATE_ADDRESS,
                              initial_seed, read_random_state, read_crt_random_state)
from compare_pal4_random_states import compare
from compare_pal4_combat_states import compare as compare_combat


class ObservationTests(unittest.TestCase):
    def test_crt_word_is_separate_and_accepts_zero_or_unsigned_full_range(self):
        for word in (0, 0xFFFFFFFF):
            calls = []
            def read(address, size, *, ida):
                calls.append((address, size, ida))
                return struct.pack('<I', word)
            self.assertEqual(read_crt_random_state(read)['state_word'], word)
            self.assertEqual(calls, [(CRT_STATE_ADDRESS, 4, True)] * 2)

    def test_changing_crt_word_retries_but_never_reports_unstable_state(self):
        values = iter([1, 2, 3, 3])
        self.assertEqual(read_crt_random_state(lambda *a, **kw: struct.pack('<I', next(values)))['state_word'], 3)
        calls = []
        def changing(*args, **kwargs):
            calls.append(args)
            return struct.pack('<I', len(calls))
        with self.assertRaises(RuntimeError):
            read_crt_random_state(changing)
        self.assertEqual(len(calls), 6)

    def test_zero_seed_is_a_real_valid_initial_state(self):
        calls = []
        def read(address, size, *, ida):
            calls.append((address, size, ida))
            return struct.pack('<I', 624) if address == INDEX_ADDRESS else bytes(624 * 4)
        value = read_random_state(read)
        self.assertEqual(initial_seed(value), 0)
        self.assertEqual(len(calls), 5)
        self.assertTrue(all(ida for _, _, ida in calls))

    def test_consumed_or_damaged_state_is_not_a_startup_seed(self):
        with self.assertRaises(RuntimeError):
            initial_seed(dict(index=1, state_words=[0] * 624))
        with self.assertRaises(RuntimeError):
            initial_seed(dict(index=624, state_words=[1] * 624))

    def test_unstable_reader_has_a_bound_and_cannot_report_false_equality(self):
        index = 0
        def read(address, size, *, ida):
            nonlocal index
            if address == INDEX_ADDRESS:
                index += 1
                return struct.pack('<I', index)
            self.assertEqual((address, size), (STATE_ADDRESS, 2496))
            return bytes(size)
        with self.assertRaises(RuntimeError):
            read_random_state(read)
        self.assertEqual(index, 9)

    def test_index_and_hidden_state_difference_are_distinct(self):
        left = dict(random_state=dict(index=3, state_words=[0] * 624))
        right = dict(random_state=dict(index=5, state_words=[0] * 624))
        value = compare(left, right)
        self.assertTrue(value['state_words_equal'])
        self.assertFalse(value['stream_position_equal'])
        self.assertEqual(value['native_index_minus_original'], 2)
        right['random_state']['state_words'][623] = 1
        value = compare(left, right)
        self.assertFalse(value['state_words_equal'])
        self.assertEqual(value['first_different_state_word'], 623)
        self.assertIsNone(value['native_index_minus_original'])

    def test_duplicate_monsters_are_not_reordered_to_hide_different_damage(self):
        def actor(hp, player=False):
            return dict(id=2501, player=player, current_hp_sp_mp=[hp, 15, 30],
                        turn_count=0, ready828=1, action660=0, action_progress=10.0,
                        timed_skills=[], permanent_skills=[])
        original = dict(combat=dict(players=[], monsters=[actor(0), actor(30)], state=3, mode=0))
        native = dict(combat=dict(actors=[actor(30), actor(0)], state=3, mode=0))
        rows = compare_combat(original, native)['actors']
        self.assertEqual([row['index'] for row in rows], [0, 1])
        self.assertTrue(all(not row['fields']['current_hp_sp_mp']['equal'] for row in rows))


if __name__ == '__main__':
    unittest.main()
