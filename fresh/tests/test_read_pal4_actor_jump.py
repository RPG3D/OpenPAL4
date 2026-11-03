import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from read_pal4_actor_jump import sample


class JumpReaderTests(unittest.TestCase):
    def test_member_addresses_normalize_across_processes(self):
        for base in (0x1000, 0x12345000):
            for offset, expected in [(0, 'none'), (140, 'jump'), (660, 'motion'), (4, 'other')]:
                calls = []
                def read(address, size):
                    calls.append((address, size))
                    if address == base + 732:
                        return struct.pack('<I', base + offset if offset else 0)
                    self.assertEqual((address, size), (base + 188, 92))
                    return struct.pack('<23I', *range(12, 35))
                result = sample(read, base)
                self.assertEqual(result, dict(active_component=expected,
                                             database_words_12_34=list(range(12, 35))))
                self.assertEqual(calls, [(base + 732, 4), (base + 188, 92)])


if __name__ == '__main__':
    unittest.main()
