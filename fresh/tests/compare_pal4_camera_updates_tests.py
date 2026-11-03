import sys
import unittest
from copy import deepcopy
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from compare_pal4_camera_updates import compare


def capture(leading=0):
    frames = []
    for i in range(16):
        image = [0]*187
        image[0x3c//4] = i-leading+100  # unique input for each simulated call
        frames.append(dict(before=image, after=list(image), after_advance=list(image),
                           has_delta=True, delta_bits=1015759766, advance_complete=True,
                           aborted=False, dropped=0, result=1, queries=[dict(kind=1, ray=[0]*6, result=1, hit=[0]*4,
                               fp_before=dict(x87=127, mxcsr=0x1f80), fp_after=dict(x87=127, mxcsr=0x1f80))],
                           fp_before=dict(x87=127, mxcsr=0x1f80), fp_after=dict(x87=127, mxcsr=0x1f80)))
    return dict(schema=2, request=123, enabled=True, count=16, pending=False, nested=0, skipped=0, frames=frames)


class CompareCameraUpdates(unittest.TestCase):
    def test_startup_waiting_counters_do_not_prove_outer_frame_alignment(self):
        a, b = capture(), capture()
        for data in (a, b):
            for frame in data['frames']:
                frame.update(frame_gate_enabled=True, outer_completed=0,
                             outer_callbacks=0, outer_depth=1)
        result = compare(a, b, same_index=True)
        self.assertTrue(result['passed'])
        self.assertFalse(result['outer_frame_verified'])
        self.assertEqual(result['alignment_basis'], 'call_index_only')

    def test_aligns_inputs_and_reports_unmatched_calls(self):
        result = compare(capture(4), capture())
        self.assertTrue(result['passed']); self.assertEqual(result['anchor'], dict(original=4, native=0))
        self.assertEqual(len(result['records']), 12)

    def test_first_output_failure_is_not_skipped(self):
        a, b = capture(4), capture(); b['frames'][0]['after_advance'][0x68//4] = 1
        result = compare(a, b)
        self.assertFalse(result['passed']); self.assertEqual(result['status'], 'post_update_divergence')
        self.assertEqual(result['first_difference'], 0)

    def test_rejects_old_or_incomplete_capture(self):
        for field, value in [('schema', 1), ('pending', True), ('nested', 1)]:
            data = capture(); data[field] = value
            with self.assertRaises(ValueError): compare(data, capture())
        data = capture(); data['frames'][0]['advance_complete'] = False
        with self.assertRaises(ValueError): compare(data, capture())

    def test_does_not_resynchronize_after_input_fork(self):
        a, b = capture(4), capture(); b['frames'][1]['before'][0x3c//4] = 999
        result = compare(a, b)
        self.assertEqual(result['status'], 'subsequent_input_divergence'); self.assertFalse(result['passed'])

    def test_query_output_difference_fails(self):
        a, b = capture(4), capture(); b['frames'][0]['queries'][0]['hit'][0] = 1
        self.assertEqual(compare(a, b)['status'], 'post_update_divergence')

    def test_rejects_wrong_request_and_malformed_query(self):
        b = capture(); b['request'] = 124
        with self.assertRaises(ValueError): compare(capture(), b)
        b = capture(); b['frames'][0]['queries'][0]['ray'] = []
        with self.assertRaises(ValueError): compare(capture(), b)

    def test_no_hit_needs_identical_cache(self):
        a, b = capture(4), capture(); a['frames'][4]['before'][0x2b8//4] = 1
        self.assertTrue(compare(a, b)['passed'])  # success overwrites prior collision cache
        for data, index in [(a, 4), (b, 0)]: data['frames'][index]['result'] = 0
        self.assertEqual(compare(a, b)['status'], 'fallback_cache_inputs_differ')

    def test_rejects_nonfinite_numerical_evidence(self):
        for field, index in [('ray', 0), ('hit', 3)]:
            data = capture(); data['frames'][0]['queries'][0][field][index] = 0x7fc00000
            with self.assertRaises(ValueError): compare(data, data)
        data = capture(); data['frames'][0]['after_advance'][0xa8//4] = 0x7f800000
        with self.assertRaises(ValueError): compare(data, data)
        for delta in [None, -1, True, 0x7fc00000, 0xbf800000]:
            data = capture(); data['frames'][0]['delta_bits'] = delta
            with self.assertRaises(ValueError): compare(data, data)

    def test_ignores_unused_no_hit_payload(self):
        data = capture(); query = data['frames'][0]['queries'][0]
        query['result'] = 0; query['hit'] = [0x7fc00000]*4
        self.assertTrue(compare(data, data)['passed'])

    def test_same_index_equal_valid_capture_covers_all_sixteen(self):
        result = compare(capture(), capture(), same_index=True)
        self.assertTrue(result['passed'])
        self.assertEqual(result['anchor'], dict(original=0, native=0))
        self.assertEqual(len(result['records']), 16)
        self.assertEqual(result['status'], 'same_index_equal')
        self.assertEqual(result['alignment_basis'], 'call_index_only')
        self.assertFalse(result['outer_frame_verified'])

    def test_same_index_does_not_skip_initial_input_or_ray_fork(self):
        for field in ('input', 'ray', 'delta'):
            with self.subTest(field=field):
                a, b = capture(), capture()
                if field == 'input': b['frames'][0]['before'][0x48//4] = 1
                elif field == 'ray': b['frames'][0]['queries'][0]['ray'][2] = 1
                else: b['frames'][0]['delta_bits'] += 1
                self.assertTrue(compare(a, b)['passed'])  # legacy excludes call zero
                result = compare(a, b, same_index=True)
                self.assertFalse(result['passed'])
                self.assertEqual(result['first_difference'], 0)
                self.assertEqual(result['anchor'], dict(original=0, native=0))
                self.assertEqual(result['difference']['stage'], 'ray' if field == 'ray' else 'before')

    def test_same_index_never_hides_later_fork(self):
        a, b = capture(), capture()
        b['frames'][5]['queries'][0]['hit'][3] = 1
        result = compare(a, b, same_index=True)
        self.assertEqual(result['first_difference'], 5)
        self.assertEqual(len(result['records']), 5)
        self.assertEqual(result['difference'], dict(stage='hit', field='point', original=0,
            native=1, query=0, component=3))

    def test_same_index_reports_each_observation_layer(self):
        changes = [
            ('before_cache', lambda f: f['before'].__setitem__(0x2d4//4, 1)),
            ('ray_shape', lambda f: f['queries'].append(deepcopy(f['queries'][0]))),
            ('hit', lambda f: f['queries'][0].__setitem__('result', 0)),
            ('after_collision', lambda f: f['after'].__setitem__(0x2e8//4, 1)),
            ('after_advance', lambda f: f['after_advance'].__setitem__(0x68//4, 1)),
            ('after_advance', lambda f: f['after_advance'].__setitem__(0xa8//4, 1)),
        ]
        for stage, change in changes:
            with self.subTest(stage=stage):
                a, b = capture(), capture(); change(b['frames'][0])
                result = compare(a, b, same_index=True)
                self.assertFalse(result['passed']); self.assertEqual(result['first_difference'], 0)
                self.assertEqual(result['difference']['stage'], stage)

    def test_same_index_checks_fp_before_after_and_query_controls(self):
        for scope, field in [('before', 'fp_before'), ('after_collision', 'fp_after'),
                             ('query_before', 'fp_before'), ('query_after', 'fp_after')]:
            with self.subTest(scope=scope):
                a, b = capture(), capture()
                receiver = b['frames'][0]['queries'][0] if scope.startswith('query') else b['frames'][0]
                receiver[field]['x87'] = 0x27f
                result = compare(a, b, same_index=True)
                self.assertEqual(result['difference']['stage'], scope)
                self.assertEqual(result['difference']['field'], 'x87')
        a, b = capture(), capture(); b['frames'][0]['fp_after']['mxcsr'] |= 0x3f
        self.assertTrue(compare(a, b, same_index=True)['passed'])
        b['frames'][0]['fp_after']['mxcsr'] |= 0x2000
        self.assertFalse(compare(a, b, same_index=True)['passed'])

    def test_same_index_excludes_process_pointers_padding_and_invalid_payloads(self):
        a, b = capture(), capture()
        b['frames'][0]['camera'] = 0x12345678
        for field in ('before', 'after', 'after_advance'):
            b['frames'][0][field][0] = 0x12345678
            b['frames'][0][field][0x2bc//4] = 0x12345678
        b['frames'][0]['before'][0x2b8//4] = 0x5e8000  # inactive null-function sentinel
        b['frames'][0]['after'][0x2c0//4] = 0xabcdef00  # only low flag byte is meaningful
        b['frames'][0]['after_advance'][0xb4//4] = 0xffffffff  # matrix padding/flags
        for data in (a, b): data['frames'][0]['queries'][0]['result'] = 0
        b['frames'][0]['queries'][0].update(owner=12, triangle=34, atomic=56, hit=[0x7fc00000]*4)
        self.assertTrue(compare(a, b, same_index=True)['passed'])

    def test_same_index_compares_valid_cache_and_collision_payloads(self):
        for field, stage in [('before', 'before_cache'), ('after', 'after_collision')]:
            a, b = capture(), capture()
            for data in (a, b): data['frames'][0][field][0x2d4//4] = 1
            b['frames'][0][field][0x2dc//4] = 1
            self.assertEqual(compare(a, b, same_index=True)['difference']['stage'], stage)
        a, b = capture(), capture()
        a['frames'][0]['after'][0x2c0//4] = 0xabcdefF8
        b['frames'][0]['after'][0x2c0//4] = 1
        self.assertTrue(compare(a, b, same_index=True)['passed'])

    def test_same_index_rejects_missing_fp_evidence_and_skipped_calls(self):
        a = capture(); del a['frames'][0]['queries'][0]['fp_after']
        with self.assertRaises(ValueError): compare(a, capture(), same_index=True)
        a = capture(); a['skipped'] = 1
        result = compare(a, capture(), same_index=True)
        self.assertFalse(result['passed']); self.assertEqual(result['status'], 'capture_has_skipped_calls')

    def test_same_index_requires_same_outer_frame_when_metadata_present(self):
        a, b = capture(), capture()
        for data in (a, b):
            for index, frame in enumerate(data['frames']):
                frame.update(frame_gate_enabled=True, outer_completed=7617+index, outer_depth=1, outer_callbacks=7618+index)
        result = compare(a, b, same_index=True)
        self.assertTrue(result['passed']); self.assertTrue(result['outer_frame_verified'])
        for field in ('outer_completed', 'outer_depth', 'outer_callbacks'):
            with self.subTest(field=field):
                fork = deepcopy(b); fork['frames'][0][field] += 1
                result = compare(a, fork, same_index=True)
                self.assertFalse(result['passed']); self.assertEqual(result['first_difference'], 0)
                self.assertEqual(result['difference']['stage'], 'outer_frame')
                self.assertEqual(result['difference']['field'], field)

    def test_same_index_never_treats_missing_frame_metadata_as_alignment(self):
        a, b = capture(), capture()
        a['frames'][0].update(frame_gate_enabled=True, outer_completed=1, outer_depth=1, outer_callbacks=2)
        result = compare(a, b, same_index=True)
        self.assertFalse(result['passed']); self.assertFalse(result['outer_frame_verified'])
        self.assertEqual(result['status'], 'missing_framegate_evidence')

    def test_disabled_or_outside_gate_does_not_claim_outer_frame_verification(self):
        a, b = capture(), capture()
        for data in (a, b):
            for frame in data['frames']:
                frame.update(frame_gate_enabled=False, outer_completed=0, outer_depth=0, outer_callbacks=0)
        result = compare(a, b, same_index=True)
        self.assertTrue(result['passed']); self.assertFalse(result['outer_frame_verified'])
        for data in (a, b):
            for frame in data['frames']: frame['frame_gate_enabled'] = True
        self.assertEqual(compare(a, b, same_index=True)['status'], 'outside_outer_frame')


if __name__ == '__main__': unittest.main()
