import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from pal4_game_heap_trace import events, pointer_evidence, read_requested_trace


def row(sequence, kind, before=0, after=0, usable_before=0, usable_after=0):
    return dict(sequence=sequence, kind=kind, before=before, after=after,
                usable_before=usable_before, usable_after=usable_after,
                size=168 if kind == 0 else 0, secondary=0, caller=0x63E390,
                thread=1, source=None, source_line=0)


class HeapTraceTests(unittest.TestCase):
    def fixture(self):
        records = [row(0, 0, after=100, usable_after=172),
                   row(1, 1, before=100, usable_before=172),
                   row(2, 0, after=100, usable_after=172)]
        return dict(enabled=True, total=3,
                    recent=dict(capacity=2, count=2, dropped=1, records=records[1:]),
                    image_candidates=dict(capacity=4, count=3, dropped=0, records=records))

    def test_reuse_and_partial_history_are_explicit(self):
        evidence = pointer_evidence(self.fixture(), 100)
        self.assertEqual([r["kind"] for r in evidence["events"]], [0, 1, 0])
        self.assertEqual(evidence["recent_dropped"], 1)
        self.assertEqual(evidence["candidate_dropped"], 0)

    def test_overlapping_windows_must_agree(self):
        trace = self.fixture()
        trace["recent"]["records"] = copy.deepcopy(trace["recent"]["records"])
        trace["recent"]["records"][0]["caller"] += 1
        with self.assertRaisesRegex(ValueError, "overlapping"):
            events(trace)

    def test_out_of_order_or_missing_records_rejected(self):
        trace = self.fixture()
        trace["recent"]["records"].reverse()
        with self.assertRaisesRegex(ValueError, "completion order"):
            events(trace)
        trace = self.fixture()
        trace["recent"]["count"] = 1
        with self.assertRaisesRegex(ValueError, "record count"):
            events(trace)

    def test_disabled_trace_is_not_evidence(self):
        trace = self.fixture()
        trace["enabled"] = False
        with self.assertRaisesRegex(ValueError, "not enabled"):
            events(trace)

    def test_reader_uses_explicit_trace_request_and_process_identity(self):
        with tempfile.TemporaryDirectory(dir="H:/PAL4/objs") as temporary:
            root = Path(temporary)
            heap = root / "compare-original-game-heap-trace-1234-37.json"
            heap.write_text(json.dumps(self.fixture()))
            result = read_requested_trace(root, "original", 1234, 37)
            self.assertEqual(Path(result["path"]), heap)
            with self.assertRaises(FileNotFoundError):
                read_requested_trace(root, "original", 1234, 38)
            with self.assertRaisesRegex(ValueError, "PID"):
                read_requested_trace(root, "original", False, 37)


if __name__ == "__main__":
    unittest.main()
