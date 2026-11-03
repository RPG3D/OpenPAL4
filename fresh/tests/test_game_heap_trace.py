import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from pal4_game_heap_trace import events, pointer_evidence, read_requested_trace, backend_events, backend_failure_events


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

    def test_backend_requires_observation_and_preserves_failed_call(self):
        trace = self.fixture()
        with self.assertRaisesRegex(ValueError, "not observed"):
            backend_events(trace)
        failed = dict(sequence=8, kind=4, size_bits=4096, requested=0,
                      result=0xFFFFFFFF, top=100, properties=1, caller=0x402A0A,
                      thread=1, heap_completed=2)
        fallback = dict(failed, sequence=9, kind=5, size_bits=1048576, result=1000)
        trace["backend"] = dict(capacity=2, count=2, dropped=8, records=[failed, fallback])
        self.assertEqual(backend_events(trace), [failed, fallback])
        self.assertEqual(trace["backend"]["dropped"], 8)
        failed["windows_ui"] = 0
        with self.assertRaisesRegex(ValueError, "module"):
            backend_events(trace)
        failed["core_messaging"] = 0x67950000
        self.assertEqual(backend_events(trace), [failed, fallback])
        fallback["heap_completed"] = 4
        with self.assertRaisesRegex(ValueError, "anchor"):
            backend_events(trace)
        fallback["heap_completed"] = 2
        fallback["sequence"] = 10
        with self.assertRaisesRegex(ValueError, "sequence"):
            backend_events(trace)

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

    def test_failure_context_must_match_actual_failed_positive_morecore(self):
        trace = self.fixture()
        call = dict(sequence=0, kind=4, size_bits=4096, requested=0, result=0xFFFFFFFF,
                    top=100, properties=1, caller=0x402A05, thread=1, heap_completed=2)
        trace["backend"] = dict(capacity=2, count=1, dropped=0, records=[call])
        with self.assertRaisesRegex(ValueError, "not observed"):
            backend_failure_events(trace)
        failure = dict(backend_sequence=0, heap_completed=2, stack=[123,456],
                       query_valid=0, top_region=[0]*7, after_reservation=[0]*7)
        trace["backend_failures"] = dict(capacity=128, count=1, dropped=0, records=[failure])
        self.assertEqual(backend_failure_events(trace), [failure])
        call["result"] = 1000
        with self.assertRaisesRegex(ValueError, "disagrees"):
            backend_failure_events(trace)
        call["result"] = 0xFFFFFFFF
        failure["top_region"] = [0]*6
        with self.assertRaisesRegex(ValueError, "length"):
            backend_failure_events(trace)


if __name__ == "__main__":
    unittest.main()
