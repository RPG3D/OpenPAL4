import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from pal4_pair_console import Pair, summarize
from pal4_combat_controls import sample as sample_controls
from pal4_party_stats import read_party


def actor(address, actor_id, player=True):
    return {'address': address, 'id': actor_id, 'player': player,
            'current_hp_sp_mp': [100, 10, 20], 'turn_count': 1,
            'ready828': 1, 'action660': 0, 'action_progress': 64.0,
            'position': [0, 0, 0], 'direction': [0, 0, 0],
            'timed_skills': [], 'permanent_skills': []}


class ControlMemory:
    def __init__(self):
        self.data = bytearray(0x200000)
        self.header_reads = 0
        self.race_header = False
        for rva, signature in ((0x749c0, '8b81e0040000'), (0x755c0, '8b81c0040000'),
                               (0x177b0, '8d4104c3')):
            self.put(0x100000 + rva, bytes.fromhex(signature))
        for address, value in ((0x2000, 0x845338), (0x2028, 0x845310), (0x2058, 0x3000),
                               (0x3000 + 804, 1), (0x3000 + 1196, 1003),
                               (0x21e0, 0x4000), (0x205c, 0x8455f4),
                               (0x205c + 36, 0x7000), (0x205c + 40, 0x6000),
                               (0x205c + 44, 0x8000), (0x64e0, 0x9000),
                               (0x64e4, 0x9018), (0x9004, 0xa000), (0xa000, 0xb000),
                               (0xb000 + 312, 5000)):
            self.word(address, value)
        self.put(0x4155, b'\x01')
        self.put(0x7155, b'\x01')
        self.string(0xb004, 'Ice')
        self.string(0x804c, 'Target')

    def put(self, address, data):
        self.data[address:address + len(data)] = data

    def word(self, address, value):
        self.put(address, struct.pack('<I', value))

    def string(self, address, value):
        self.put(address, struct.pack('<2I', len(value), 32))
        self.put(address + 20, value.encode('utf-32-le') + bytes(4))

    def read(self, address, size):
        if address == 0x64e0 and size == 8:
            self.header_reads += 1
            if self.race_header and self.header_reads == 2:
                return struct.pack('<2I', 0x9000, 0x9030)
        return bytes(self.data[address:address + size])


class ConsoleTests(unittest.TestCase):
    def test_persistent_party_reads_all_slots_without_target_calls(self):
        memory = bytearray(0x2c64)
        def put(offset, value):
            struct.pack_into('<I', memory, offset, value)
        put(11344, 2)
        put(2836 + 2824, 1)
        component = 2836 + 1620
        for index, value in ((143, 716), (144, 100), (145, 111), (86, 1110),
                             (88, 120), (140, 18), (162, 0)):
            put(component + 4 * index, value)
        reader = Mock(return_value=bytes(memory))
        party = read_party(reader, 0x1000)
        reader.assert_called_once_with(0x1000, 0x2c64)
        self.assertEqual(party['leader_index'], 2)
        self.assertEqual(len(party['players']), 4)
        self.assertEqual(party['players'][1], dict(index=1, active=1,
            current_hp_sp_mp=[716, 100, 111], max_hp_mp=[1110, 120], level=18, health_status=0))
        with self.assertRaisesRegex(ValueError, 'missing'):
            read_party(reader, 0)
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            read_party(lambda *_: bytes(memory[:-1]), 0x1000)
        put(11344, 4)
        with self.assertRaisesRegex(ValueError, 'leader'):
            read_party(lambda *_: bytes(memory), 0x1000)

    def test_persistent_party_exposes_save_exit_hp_fork_without_combat_rosters(self):
        party = dict(leader_index=0, players=[dict(index=i, active=1,
            current_hp_sp_mp=[1110, 100, 111], max_hp_mp=[1110, 111], level=18,
            health_status=0) for i in range(4)])
        other = json.loads(json.dumps(party))
        self.assertTrue(summarize({'party': party}, {'party': other})['party']['equal'])
        other['players'][1]['current_hp_sp_mp'][0] = 716
        self.assertFalse(summarize({'party': party}, {'party': other})['party']['equal'])
        self.assertFalse(summarize({'party': party}, {})['party']['equal'])
        other['players'].pop()
        with self.assertRaisesRegex(ValueError, 'four ordered'):
            summarize({'party': party}, {'party': other})

    def test_interaction_occupancy_preserves_stuck_target_without_equating_addresses(self):
        left = {'interaction_targets': [0, 0, 0]}
        right = {'interaction_targets': [0, 0, 1234]}
        self.assertFalse(summarize(left, right)['interaction_target_active']['equal'])
        left['interaction_targets'][2] = 5678
        self.assertTrue(summarize(left, right)['interaction_target_active']['equal'])
        self.assertNotIn('interaction_target_active', summarize(left, {}))
        with self.assertRaisesRegex(ValueError, 'NPC/GOB/EVF'):
            summarize(left, {'interaction_targets': []})

    def test_script_globals_preserve_story_and_named_differences(self):
        left = {'scripts': {'global_variables': [0] * 48}}
        right = {'scripts': {'global_variables': [0] * 48}}
        self.assertNotIn('script_globals', summarize(left, {'scripts': {}}))
        self.assertTrue(summarize(left, right)['script_globals']['equal'])
        right['scripts']['global_variables'][16] = -1  # var_mo, not an unsigned token
        result = summarize(left, right)['script_globals']
        self.assertEqual(result['differences'], [dict(original_address='0x90df00', original=0, native=-1)])
        right['scripts']['global_variables'].pop()
        with self.assertRaisesRegex(ValueError, '48'):
            summarize(left, right)

    def test_control_observer_reads_actor_and_selected_skill(self):
        value = sample_controls(ControlMemory(), 0x2000, 0x100000, 'magic')
        self.assertEqual(value['actor'], 1003)
        self.assertTrue(value['panel_visible'])
        self.assertEqual(value['selector']['rows'][0]['id'], 5000)
        self.assertEqual(value['selector']['rows'][0]['text'], 'Ice')

    def test_control_observer_rejects_changed_dll_and_invalid_bounds(self):
        memory = ControlMemory()
        memory.put(0x1749c0, b'\x00')
        with self.assertRaisesRegex(ValueError, 'signature'):
            sample_controls(memory, 0x2000, 0x100000, 'magic')
        memory = ControlMemory()
        memory.word(0x64e4, 0x9001)
        with self.assertRaisesRegex(ValueError, 'row vector'):
            sample_controls(memory, 0x2000, 0x100000, 'magic')

    def test_control_observer_rejects_raced_list(self):
        memory = ControlMemory()
        memory.race_header = True
        with self.assertRaisesRegex(RuntimeError, 'changed during'):
            sample_controls(memory, 0x2000, 0x100000, 'magic')

    def test_summary_keeps_actor_and_full_rng_differences(self):
        original_actor = actor(100, 1001)
        native_actor = actor(200, 1001)
        original = {'world_state_active': False, 'position': [1, 2, 3],
                    'random_state': {'index': 7, 'state_words': [0] * 624},
                    'combat': {'state': 4, 'mode': 0, 'players': [original_actor], 'monsters': []}}
        native = {'world_state_active': False, 'position': [1, 2, 3],
                  'random_state': {'index': 7, 'state_words': [0] * 624},
                  'combat': {'state': 4, 'mode': 0, 'actors': [native_actor]},
                  'scripts': {'world': 'M07', 'scene': '6'}}
        native['random_state']['state_words'][400] = 9
        native_actor['current_hp_sp_mp'][0] = 99
        result = summarize(original, native)
        self.assertFalse(result['random']['stream_position_equal'])
        self.assertEqual(result['random']['first_different_state_word'], 400)
        self.assertFalse(result['combat']['actors'][0]['fields']['current_hp_sp_mp']['equal'])
        self.assertIsNone(result['scripts_original'])

    def test_input_failure_records_only_successful_sends(self):
        with tempfile.TemporaryDirectory() as directory:
            pair = Pair.__new__(Pair)
            pair.root = Path(directory)
            pair.user = Mock()
            pair.user.MapVirtualKeyW.return_value = 28
            pair.request = lambda: 123
            pair.validate = Mock()
            pair.post = Mock(side_effect=[None, RuntimeError('second side failed'), None])
            with self.assertRaisesRegex(RuntimeError, 'second side'):
                pair.key('ENTER')
            events = json.loads((pair.root / 'console-input-123.json').read_text())
            self.assertEqual([(e['side'], e['message']) for e in events],
                             [('original', 0x100), ('original', 0x101)])
            self.assertEqual(pair.post.call_count, 3)

    def test_dialogue_mismatch_never_sends_input(self):
        pair = Pair.__new__(Pair)
        pair.dialogue = Mock(return_value={'text_equal': False, 'ready': True, 'same_page_state': True})
        pair.key = Mock()
        with self.assertRaisesRegex(RuntimeError, 'differs'):
            pair.advance_dialogue(123)
        pair.key.assert_not_called()


if __name__ == '__main__':
    unittest.main()
