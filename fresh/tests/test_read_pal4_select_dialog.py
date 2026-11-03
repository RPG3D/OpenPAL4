import struct
import sys
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from read_pal4_select_dialog import sample

class Reader:
    def __init__(self):
        self.memory=bytearray(0x6000)
        self.word(0x1000,0x84434c);self.word(0x1028,0x844364)
        for off,value in [(36,0x2000),(48,0x3000),(52,0x3008),(60,0),(132,0x4000),(156,0xffffffff),(160,1)]:self.word(0x1000+off,value)
        self.word(0x2044,0x5000);self.word(0x3000,0x4100);self.word(0x3004,0x4200)
        self.word(0x4100+280,0);self.word(0x4200+280,1)
        self.changed=False;self.samples=0
    def word(self,a,v):struct.pack_into('<I',self.memory,a,v)
    def read(self,a,n):
        if a==0x1000 and n==164:
            self.samples+=1
            if self.changed and self.samples>1:self.word(0x103c,1)
        return bytes(self.memory[a:a+n])

class SelectReaderTests(unittest.TestCase):
    def read(self,r):
        with patch('read_pal4_select_dialog.read_window_text',side_effect=lambda _,p:{0x4000:'Question',0x4100:'Yes',0x4200:'No'}[p]):return sample(r,0x1000)
    def test_signed_result_and_attached_options(self):
        s=self.read(Reader());self.assertEqual(s['last_selection'],-1);self.assertEqual(s['escape_default'],1);self.assertTrue(s['root_attached']);self.assertEqual(s['options'],[dict(text='Yes',index=0),dict(text='No',index=1)])
    def test_hidden_cache_is_reported_separately(self):
        r=Reader();r.word(0x2044,0);s=self.read(r);self.assertFalse(s['root_attached']);self.assertEqual(len(s['options']),2)
    def test_bad_range_and_index_are_rejected(self):
        for field,value in [(52,0x2fff),(52,0x9000),(48,0),(0x4200+280-0x1000,8)]:
            r=Reader();r.word(0x1000+field,value)
            with self.assertRaises(ValueError):self.read(r)
    def test_changed_frame_is_not_accepted(self):
        r=Reader();r.changed=True
        with self.assertRaisesRegex(RuntimeError,'changed while sampled'):self.read(r)

if __name__=='__main__':unittest.main()
