import struct
import tempfile
import unittest
import zipfile
from pathlib import Path

from audit_release import audit_artifact


def make_nro(nacp=b"N" * 64):
    image_size = 0x80
    data = bytearray(image_size + 56 + len(nacp))
    data[0x10:0x14] = b"NRO0"
    struct.pack_into("<III", data, 0x14, 0, image_size, 0)
    for header_offset, offset, size in (
        (0x20, 0, 0x40),
        (0x28, 0x40, 0x20),
        (0x30, 0x60, 0x20),
    ):
        struct.pack_into("<II", data, header_offset, offset, size)
    data[image_size:image_size + 4] = b"ASET"
    struct.pack_into("<I6Q", data, image_size + 4, 0, 0, 0, 56, len(nacp), 120 + len(nacp), 0)
    data[image_size + 56:] = nacp
    return data


class ReleaseAuditTests(unittest.TestCase):
    def test_extension_screen_and_binary_signature_are_separate(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "runtime.zip"
            with zipfile.ZipFile(package, "w") as archive:
                archive.writestr("assets/scene.cpk", b"ordinary payload")
                archive.writestr("lib/opaque.bin", b"prefix CPK suffix")

            report = audit_artifact(package)

        self.assertTrue(report["extension_screen_found_assets"])
        self.assertTrue(report["candidate_byte_markers_found"])
        self.assertFalse(report["binary_absence_proven"])
        self.assertFalse(report["members"][1]["known_asset_extension"])
        self.assertEqual("CPK-like bytes", report["members"][1]["candidate_byte_markers"][0]["marker"])

    def test_nro_uses_declared_image_end_and_ignores_fake_aset(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "runtime.nro"
            nacp = bytearray(b"N" * 64)
            nacp[40:44] = b"ASET"
            data = make_nro(nacp)
            package.write_bytes(data)

            report = audit_artifact(package)

        nro = report["members"][0]["nro"]
        self.assertEqual(0x80, nro["aset_offset"])
        self.assertEqual(0, nro["romfs_size"])
        self.assertEqual(["text", "ro", "data"], [segment["name"] for segment in nro["segments"]])

    def test_nro_rejects_each_segment_outside_image(self):
        for header_offset in (0x20, 0x28, 0x30):
            with self.subTest(segment_header=header_offset), tempfile.TemporaryDirectory() as directory:
                package = Path(directory) / "runtime.nro"
                data = make_nro()
                struct.pack_into("<II", data, header_offset, 0x70, 0x20)
                package.write_bytes(data)

                with self.assertRaisesRegex(ValueError, "segment exceeds"):
                    audit_artifact(package)

    def test_nro_rejects_unsupported_header_version(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "future.nro"
            data = make_nro()
            struct.pack_into("<I", data, 0x14, 1)
            package.write_bytes(data)

            with self.assertRaisesRegex(ValueError, "header version"):
                audit_artifact(package)

    def test_nro_rejects_aset_section_outside_file(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "bad-aset.nro"
            data = make_nro()
            struct.pack_into("<Q", data, 0x80 + 32, 0xFFFFFFFF)
            package.write_bytes(data)

            with self.assertRaisesRegex(ValueError, "ASET nacp section"):
                audit_artifact(package)

    def test_nro_rejects_truncated_aset_header(self):
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "truncated.nro"
            data = make_nro()[:0x80 + 20]
            package.write_bytes(data)

            with self.assertRaisesRegex(ValueError, "truncated NRO ASET"):
                audit_artifact(package)


if __name__ == "__main__":
    unittest.main()
