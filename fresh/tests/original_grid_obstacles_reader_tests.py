"""Transport fixtures verify the observer, not original candidate production."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from read_pal4_original_grid_obstacles import sample


class Reader:
    module_base = 0x500000

    def __init__(self):
        self.blocks = {0x9e42a8: struct.pack('<I',0x1000),
                       0xa0dec0: bytes(192), 0x1038: bytes(38)}
        self.header_reads = 0
        self.change_header = False

    def read(self, address, size, *, ida=False):
        address += self.module_base - 0x400000 if ida else 0
        if address == 0x1038:
            self.header_reads += 1
            if self.change_header and self.header_reads == 2:
                return bytes([1]) + self.blocks[address][1:]
        for start, block in self.blocks.items():
            if start <= address and address + size <= start + len(block):
                return block[address-start:address-start+size]
        raise OSError('unmapped observer input')

    def header(self, *, primary=(0,0,0), secondary=(0,0,0)):
        raw = bytearray(38)
        struct.pack_into('<3I',raw,4,*primary)
        struct.pack_into('<3I',raw,20,*secondary)
        self.blocks[0x1038] = bytes(raw)


class Tests(unittest.TestCase):
    def test_empty_is_actual_member_state(self):
        result = sample(Reader())
        self.assertEqual(result['context'],0x1048)
        self.assertEqual(result['secondary']['ordered_pointers'],[])
        self.assertEqual(result['records'],[])

    def test_nonempty_preserves_order_duplicates_and_opaque_word(self):
        reader = Reader()
        reader.header(secondary=(0x2000,0x200c,0x2010))
        reader.blocks[0x2000] = struct.pack('<4I',0x4000,0x5000,0x4000,0)
        reader.blocks[0x4000] = struct.pack('<I4f',0xfedcba98,2,3,4,5)
        reader.blocks[0x5000] = struct.pack('<I4f',7,6,8,9,10)
        result = sample(reader)
        self.assertEqual([r['address'] for r in result['records']],
                         [0x4000,0x5000,0x4000])
        self.assertEqual(result['records'][0]['word0'],0xfedcba98)
        self.assertEqual(result['records'][0]['center'],[3,4,5])

    def test_aslr_and_exact_helper_extents(self):
        for table, extent in [(0x842898,0xe0),(0x842978,0xc0),
                              (0x842a30,0xbc),(0x842ae8,0xe0)]:
            reader = Reader()
            reader.header(primary=(0x2000,0x2004,0x2004))
            reader.blocks[0x2000] = struct.pack('<I',0x3000)
            raw = bytearray(extent)
            struct.pack_into('<I',raw,0,table+0x100000)
            struct.pack_into('<3f',raw,0x28,1,2,3)
            reader.blocks[0x3000] = raw
            reader.blocks[table+0x100000+0xa4] = struct.pack('<I',0x52a760)
            helper = sample(reader)['helpers'][0]
            self.assertEqual(helper['position'],[1,2,3])
            self.assertEqual(len(bytes.fromhex(helper['image_hex'])),extent)

    def test_corrupt_missing_or_changed_input_never_becomes_empty(self):
        reader = Reader()
        reader.header(secondary=(0,0,4))
        with self.assertRaises(RuntimeError): sample(reader)
        reader.header(secondary=(0x2000,0x2000,0x2001))
        with self.assertRaises(RuntimeError): sample(reader)
        reader.header(secondary=(0x2000,0x2004,0x2004))
        with self.assertRaises(OSError): sample(reader)
        reader = Reader()
        reader.change_header = True
        with self.assertRaisesRegex(RuntimeError,'changed'): sample(reader)


if __name__ == '__main__':
    unittest.main()
