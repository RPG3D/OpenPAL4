import sys
import unittest
from unittest import mock
import json
import tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from pal4_pair_frame_gate import (SteppedPair, validate_receipt, require_comparable,
                                  require_checkpoint_bootstrap, CHECKPOINT_MESSAGE)


def receipt():
    return dict(pid=123,request=456,accepted=True,phase='held',depth=0,remaining=0,
        active_request=0,arm_period=100,arm_calls=1,arm_source='countdown',arm_pending=False,completed=0,granted=0,nested=0,
        clock_reads=0,callbacks=0,step_ms=17,checkpoint_allowed=False,countdown=dict(remaining=99,
        timers=[dict(initial_bits=0x3f800000,current_bits=0x3f7ba5e3)]))


class FrameGateProtocolTests(unittest.TestCase):
    def test_receipt_requires_current_process_and_request(self):
        row=receipt();self.assertEqual(validate_receipt(row,123,456),row)
        for pid,request in [(124,456),(123,457)]:
            with self.assertRaises(ValueError):validate_receipt(row,pid,request)

    def test_rejection_fault_and_invalid_counters_are_not_success(self):
        for update,error in [(dict(accepted=False),RuntimeError),(dict(phase='faulted'),RuntimeError),
                (dict(completed=True),ValueError),(dict(remaining=10001),ValueError),
                (dict(step_ms=0),ValueError)]:
            row=receipt();row.update(update)
            with self.assertRaises(error):validate_receipt(row,123,456)

    def test_initial_hold_does_not_prove_timer_phase_alignment(self):
        rows=dict(original=receipt(),native=receipt());require_comparable(rows)
        rows['native']['countdown']['timers'][0]['current_bits']+=1
        with self.assertRaisesRegex(RuntimeError,'queue phase'):require_comparable(rows)

    def test_counter_equality_does_not_accept_running_or_missing_timer(self):
        for update in [dict(phase='running'),dict(remaining=1),dict(depth=1),
                dict(countdown=None),dict(callbacks=1)]:
            rows=dict(original=receipt(),native=receipt());rows['native'].update(update)
            with self.assertRaises(RuntimeError):require_comparable(rows)

    def test_transient_publication_lock_rereads_same_credit_without_resend(self):
        class PairReader:
            root=Path('H:/PAL4/objs')
            identities=dict(original=dict(pid=123),native=dict(pid=124))
            def validate(self):pass
            def post(self,*args):raise AssertionError('must never resend a pending credit')
        reads=[]
        def read(path,**kwargs):
            reads.append(path.name)
            if 'native' in path.name and reads.count(path.name)==1:raise PermissionError('publication in progress')
            row=receipt();row['pid']=124 if 'native' in path.name else 123
            return json.dumps(row)
        with mock.patch.object(Path,'exists',return_value=True),mock.patch.object(Path,'read_text',read),mock.patch('pal4_pair_frame_gate.time.sleep'):
            rows=SteppedPair._wait_gate(PairReader(),456,1)
        self.assertEqual(set(rows),{'original','native'})
        self.assertEqual(reads.count('compare-native-framegate-124-456.json'),2)
        self.assertEqual(reads.count('compare-original-framegate-123-456.json'),1)


def checkpoint_rows(request=700):
    rows=dict(original=receipt(),native=receipt())
    for side,row in rows.items():
        row.update(request=request,active_request=request,arm_source='checkpoint',
                   arm_period=0,checkpoint_allowed=True,countdown=dict(remaining=0,timers=[]))
    return rows


class CheckpointBootstrapTests(unittest.TestCase):
    def pair(self, root, *, before=None, after=None, post_error=None, wait_error=None):
        pair=object.__new__(SteppedPair)
        pair.root=Path(root)
        pair.query_gate=mock.Mock(return_value=before or self.waiting())
        pair.request=mock.Mock(return_value=700)
        pair.post=mock.Mock(side_effect=post_error)
        pair._wait_gate=mock.Mock(return_value=after or checkpoint_rows(),side_effect=wait_error)
        return pair

    def waiting(self):
        rows=checkpoint_rows()
        for row in rows.values():
            row.update(phase='waiting',active_request=0,arm_calls=0,arm_source='none',countdown=None)
        return rows

    def test_single_send_each_side_waits_for_receipt_and_leaves_baseline_unverified(self):
        with tempfile.TemporaryDirectory() as root:
            pair=self.pair(root)
            result=pair.bootstrap_checkpoint()
            self.assertEqual(pair.post.call_args_list,[mock.call('original',CHECKPOINT_MESSAGE,700,0),mock.call('native',CHECKPOINT_MESSAGE,700,0)])
            pair._wait_gate.assert_called_once_with(700,30)
            self.assertEqual(result['status'],'held_unverified_baseline')
            self.assertEqual(result['baseline_status'],'not_evaluated')
            self.assertNotIn('passed',result)
            journal=json.loads((Path(root)/'console-framegate-checkpoint-700.json').read_text())
            self.assertEqual(journal['attempted'],['original','native'])
            self.assertEqual(journal['sent'],['original','native'])

    def test_nonwaiting_or_pending_or_unequal_step_rejects_before_send(self):
        for change in [dict(phase='held'),dict(phase='running'),dict(phase='disabled'),
                       dict(phase='faulted'),dict(arm_pending=True),dict(depth=1),
                       dict(callbacks=1),dict(remaining=1),dict(step_ms=18),
                       dict(arm_calls=1),dict(arm_source='countdown'),dict(checkpoint_allowed=False)]:
            with self.subTest(change=change),tempfile.TemporaryDirectory() as root:
                rows=self.waiting();rows['native'].update(change)
                pair=self.pair(root,before=rows)
                with self.assertRaises(RuntimeError):pair.bootstrap_checkpoint()
                pair.post.assert_not_called();pair.request.assert_not_called()

    def test_invalid_checkpoint_marker_or_nonempty_timer_never_becomes_success(self):
        for change in [dict(active_request=699),dict(arm_source='countdown'),dict(arm_period=1),
                       dict(arm_pending=True),dict(completed=1),dict(phase='running'),
                       dict(arm_calls=0),dict(checkpoint_allowed=False),
                       dict(countdown=dict(remaining=1,timers=[])),
                       dict(countdown=dict(remaining=0,timers=[dict(initial_bits=0,current_bits=0)]))]:
            with self.subTest(change=change):
                rows=checkpoint_rows();rows['native'].update(change)
                with self.assertRaises(RuntimeError):require_checkpoint_bootstrap(rows,700)

    def test_partial_send_records_attempt_before_failure_without_resend(self):
        with tempfile.TemporaryDirectory() as root:
            pair=self.pair(root,post_error=[None,OSError('post failed')])
            with self.assertRaises(OSError):pair.bootstrap_checkpoint()
            self.assertEqual(pair.post.call_count,2);pair._wait_gate.assert_not_called()
            row=json.loads((Path(root)/'console-framegate-checkpoint-700.json').read_text())
            self.assertEqual(row['attempted'],['original','native'])
            self.assertEqual(row['sent'],['original'])
            self.assertEqual(row['status'],'failed_do_not_replay')

    def test_timeout_or_rejection_is_journaled_without_bootstrap_retry(self):
        for error in [TimeoutError('pending original'),RuntimeError('frame request rejected')]:
            with self.subTest(error=error),tempfile.TemporaryDirectory() as root:
                pair=self.pair(root,wait_error=error)
                with self.assertRaises(type(error)):pair.bootstrap_checkpoint()
                self.assertEqual(pair.post.call_count,2)
                pair._wait_gate.assert_called_once_with(700,30)
                row=json.loads((Path(root)/'console-framegate-checkpoint-700.json').read_text())
                self.assertEqual(row['status'],'failed_do_not_replay')

    def test_receipt_validator_rejects_unknown_source_and_nonboolean_pending(self):
        for update in [dict(arm_source='forged'),dict(arm_pending=0),dict(checkpoint_allowed=1)]:
            row=receipt();row.update(update)
            with self.assertRaises(ValueError):validate_receipt(row,123,456)


if __name__=='__main__':unittest.main()
