# Public host source preview — 2026-11-03

Repository: https://github.com/betesla/OpenPAL4
Project-owned code: GPL-3.0-or-later; third-party terms remain applicable.

This initial public repository is a history-free portable host source selection
from development baseline 2fd8683. It includes shared runtime sources/tests,
vendored open-source dependencies, license material and host build instructions.
Original game assets, proprietary SDKs/DLLs, screenshots, original research
history and the full platform application projects are not included.

## Validation

- Independent VS2022 x64 Debug configure and full build succeeded without
  original game data, proprietary SDK inputs, IDA or previous build caches.
- 121/121 CTest entries passed in 12.67 seconds. No CTest-level skips; two
  optional original CPK/catalog and Pal4db formula subbranches skipped without
  original resources/actor operands. Synthetic contracts passed.
- Release artifact audit tool: 6/6 tests passed.
- Published C/C++/CMake build inputs match the tested snapshot byte for byte.
  Documentation, licensing and export metadata were updated afterward.
- SOURCE_MANIFEST.json records file selection and SHA-256 hashes. Its own
  hash is intentionally outside the file list.

## Limits and remaining work

Full Windows numerical acceptance reached M15/global130200; the next segment
has an effect-task RNG ordering divergence. Portable complete first chapter
and production monster/battle integration remain unfinished. These milestones
are not established by host contract tests.

Original-derived code/fixture provenance and broader distribution obligations
need continuing review. The project license does not grant rights to original
assets, proprietary dependencies or material outside project authority.

Platform binary releases require exact CEGUI/FreeType/FFmpeg/SDL/mpv/NDK/
devkitPro versions, license/source material and applicable relinking support.
CEGUI 0.4.1 is LGPL-2.1-or-later. Old incomplete binary notice bundles are not
supplied as cleared releases. No APK/NRO is part of this preview.

The full Android tree's publicRelease variant has debugging disabled and no
signing key. Only Groovy syntax was checked locally; actual Gradle configuration
was blocked by a local loopback failure.
