import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from pal4_crt_window import consume_recent


def window(start, count=3):
    return dict(enabled=True, generator='crt', retention='recent', capacity=3,
                count=count, dropped=start, start_sequence=start,
                next_sequence=start+count,
                records=[dict(sequence=i, full_sequence=i, kind=0, value=i)
                         for i in range(start, start+count)])


class RecentCrtTests(unittest.TestCase):
    def test_overlapping_receipts_cover_without_duplicates(self):
        rows, cursor = consume_recent(window(5), 5)
        more, cursor = consume_recent(window(7), cursor)
        self.assertEqual([r['value'] for r in rows+more], list(range(5, 10)))
        self.assertEqual(cursor, 10)
        self.assertEqual(consume_recent(window(7), cursor), ([], 10))

    def test_gap_and_stale_receipts_rejected(self):
        for cursor in (4, 9, True):
            with self.assertRaises(ValueError):
                consume_recent(window(5), cursor)

    def test_corrupt_ordinals_and_bounds_rejected(self):
        good = window(5)
        for field, value in [('count', 2), ('dropped', 4),
                             ('next_sequence', 9), ('capacity', 2),
                             ('retention', 'prefix'), ('enabled', False)]:
            bad = copy.deepcopy(good)
            bad[field] = value
            with self.assertRaises(ValueError):
                consume_recent(bad, 5)
        bad = copy.deepcopy(good)
        bad['records'][1]['full_sequence'] = 7
        with self.assertRaises(ValueError):
            consume_recent(bad, 5)

    def test_empty_window_and_maximum_value(self):
        self.assertEqual(consume_recent(window(5, 0), 5), ([], 5))
        item = window(5)
        item['records'][0]['value'] = 0xffffffff
        self.assertEqual(consume_recent(item, 5)[0][0]['value'], 0xffffffff)


if __name__ == '__main__':
    unittest.main()
