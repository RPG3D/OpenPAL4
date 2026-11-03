import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from pal4_pair_console import Pair, ROOT


def client(root):
    pair=object.__new__(Pair);pair.root=Path(root);return pair


class PairRequestTests(unittest.TestCase):
    def test_legacy_captures_and_relocations_cannot_be_reused(self):
        with tempfile.TemporaryDirectory(dir=ROOT) as directory:
            root=Path(directory)
            (root/'compare-native-123-43.json').write_text('{}')
            (root/'compare-relocate-request-77.txt').write_text('old')
            with mock.patch('pal4_pair_console.secrets.randbelow',side_effect=[42,76,100]):
                self.assertEqual(client(root).request(),101)
            self.assertTrue((root/'compare-request-101.reserve').exists())

    def test_another_clients_reservation_is_rejected_after_indexing(self):
        with tempfile.TemporaryDirectory(dir=ROOT) as directory:
            root=Path(directory);first,second=client(root),client(root)
            with mock.patch('pal4_pair_console.secrets.randbelow',return_value=1):
                self.assertEqual(second.request(),2)
            with mock.patch('pal4_pair_console.secrets.randbelow',return_value=42):
                self.assertEqual(first.request(),43)
            with mock.patch('pal4_pair_console.secrets.randbelow',side_effect=[42,43]):
                self.assertEqual(second.request(),44)
            self.assertTrue((root/'compare-request-43.reserve').exists())
            self.assertTrue((root/'compare-request-44.reserve').exists())


if __name__=='__main__':unittest.main()
