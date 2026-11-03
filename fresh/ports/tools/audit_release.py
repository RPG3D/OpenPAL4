#!/usr/bin/env python3
"""Offline inventory and bounded binary-signature audit for release artifacts."""

from __future__ import annotations

import argparse
import json
import struct
import zipfile
from pathlib import Path


ASSET_EXTENSIONS = {
    ".cpk", ".cs", ".csb", ".dff", ".bsp", ".dds", ".smp", ".amf",
    ".amd", ".wav", ".mp3", ".bik", ".bink", ".p4a", ".p4c", ".p4r",
    ".p4u", ".p4e", ".png", ".jpg", ".jpeg", ".ogg", ".ttf", ".fnt",
}

# These markers are only byte-sequence candidates. They can also occur in
# decoder code, tables, documentation, or unrelated data.
KNOWN_MARKERS = {
    "CPK-like bytes": b"CPK ",
    "DDS-like bytes": b"DDS ",
    "RIFF-like bytes": b"RIFF",
    "Ogg-like bytes": b"OggS",
    "PNG-signature bytes": b"\x89PNG\r\n\x1a\n",
    "JPEG-signature bytes": b"\xff\xd8\xff",
}


def _marker_hits(data: bytes) -> list[dict[str, object]]:
    hits = []
    for name, marker in KNOWN_MARKERS.items():
        offset = data.find(marker)
        if offset >= 0:
            hits.append({"marker": name, "offset": offset})
    return hits


def _nro_metadata(data: bytes) -> dict[str, object] | None:
    if len(data) < 0x14 or data[0x10:0x14] != b"NRO0":
        return None
    if len(data) < 0x80:
        raise ValueError("truncated NRO header")

    version, image_size, flags = struct.unpack_from("<III", data, 0x14)
    if version != 0:
        raise ValueError(f"unsupported NRO header version: {version}")
    if image_size < 0x80 or image_size > len(data):
        raise ValueError(f"NRO image size is outside the file: {image_size}")

    segments = []
    for name, header_offset in (("text", 0x20), ("ro", 0x28), ("data", 0x30)):
        offset, size = struct.unpack_from("<II", data, header_offset)
        end = offset + size
        if offset > image_size or end > image_size:
            raise ValueError(f"NRO {name} segment exceeds declared image size")
        if size:
            segments.append((offset, end, name))
    for previous, current in zip(sorted(segments), sorted(segments)[1:]):
        if current[0] < previous[1]:
            raise ValueError(f"NRO segments overlap: {previous[2]} and {current[2]}")

    result: dict[str, object] = {
        "nro0_header": True,
        "header_version": version,
        "image_size": image_size,
        "flags": flags,
        "segments": [
            {"name": name, "offset": start, "size": end - start}
            for start, end, name in sorted(segments)
        ],
    }

    # NRO assets begin exactly at NroHeader.size. Do not search executable or
    # asset payloads for a coincidental ASET byte sequence.
    offset = image_size
    if offset == len(data):
        result.update({"aset_found": False, "romfs_size": None})
        return result
    if data[offset:offset + 4] != b"ASET":
        result.update({
            "aset_found": False,
            "romfs_size": None,
            "unclassified_trailing_bytes": len(data) - offset,
        })
        return result
    if len(data) - offset < 56:
        raise ValueError("truncated NRO ASET header")

    aset_version = struct.unpack_from("<I", data, offset + 4)[0]
    if aset_version != 0:
        raise ValueError(f"unsupported NRO ASET version: {aset_version}")
    icon_off, icon_size, nacp_off, nacp_size, romfs_off, romfs_size = struct.unpack_from(
        "<6Q", data, offset + 8
    )
    aset_size = len(data) - offset
    sections = []
    for name, section_offset, section_size in (
        ("icon", icon_off, icon_size),
        ("nacp", nacp_off, nacp_size),
        ("romfs", romfs_off, romfs_size),
    ):
        if not section_size:
            continue
        end = section_offset + section_size
        if section_offset < 56 or end > aset_size:
            raise ValueError(f"NRO ASET {name} section exceeds asset data")
        sections.append((section_offset, end, name))
    for previous, current in zip(sorted(sections), sorted(sections)[1:]):
        if current[0] < previous[1]:
            raise ValueError(f"NRO ASET sections overlap: {previous[2]} and {current[2]}")

    result.update({
        "aset_found": True,
        "aset_offset": offset,
        "aset_version": aset_version,
        "icon_size": icon_size,
        "nacp_size": nacp_size,
        "romfs_offset": romfs_off,
        "romfs_size": romfs_size,
    })
    return result


def _inspect_bytes(name: str, data: bytes) -> dict[str, object]:
    return {
        "path": name,
        "size": len(data),
        "extension": Path(name).suffix.lower(),
        "known_asset_extension": Path(name).suffix.lower() in ASSET_EXTENSIONS,
        "candidate_byte_markers": _marker_hits(data),
        "nro": _nro_metadata(data),
    }


def audit_artifact(path: Path) -> dict[str, object]:
    path = path.resolve()
    if not path.is_file():
        raise FileNotFoundError(path)
    members: list[dict[str, object]] = []
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            for info in archive.infolist():
                if info.is_dir():
                    continue
                # Read entries independently so the report also scans executable
                # payloads whose filenames do not resemble game resources.
                members.append(_inspect_bytes(info.filename, archive.read(info)))
        container = "zip"
    else:
        members.append(_inspect_bytes(path.name, path.read_bytes()))
        container = "file"

    return {
        "artifact": str(path),
        "container": container,
        "members": members,
        "extension_screen_found_assets": any(m["known_asset_extension"] for m in members),
        "candidate_byte_markers_found": any(m["candidate_byte_markers"] for m in members),
        "binary_absence_proven": False,
        "binary_absence_limit": (
            "A bounded signature scan cannot prove absence of compressed, encrypted, "
            "transformed, split, or unknown-format game assets."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifacts", nargs="+", type=Path, help="APK, runtime ZIP, NRO, or other file")
    parser.add_argument("--output", type=Path, help="write JSON report (default: stdout)")
    args = parser.parse_args()
    report = {"schema_version": 1, "artifacts": [audit_artifact(p) for p in args.artifacts]}
    rendered = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
