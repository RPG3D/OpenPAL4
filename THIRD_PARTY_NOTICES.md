# Third-party source and release inventory

Audit date: 2026-11-03. This inventory does not replace the upstream license
texts. Project-owned code uses GPL-3.0-or-later; see `LICENSING.md`. Third-party code
retains its existing copyright notices and license terms.

## Vendored portable-runtime components

| Component | Evidence in tree | License / remaining check |
| --- | --- | --- |
| miniLZO 2.09 | `fresh/third_party/minilzo-2.09/PROVENANCE.md`, `COPYING` | GPL-2.0-or-later; linked into package reader. An MIT-only combined distribution is not established. |
| librw | `fresh/third_party/librw/LICENSE`, `UPSTREAM.json`, `PAL4_PATCHES.md` | MIT; upstream revision and local PAL4 UV animation changes are recorded. Preserve these records in source distributions. |
| AngelScript 2.38.0 | `fresh/third_party/angelscript-2.38.0/PROVENANCE.md`, `LICENSE.html` | zlib-style terms; preserve license and modification notices. |
| TinyXML 2.5.3 | `fresh/third_party/tinyxml-2.5.3/PROVENANCE.md`, source headers, `readme.txt` | zlib-style terms. |
| LodePNG | `fresh/third_party/lodepng/PROVENANCE.md`, source headers | zlib-style terms; revision/hash recorded. |
| HandmadeMath 2.0.0 | `fresh/third_party/handmade-math/PROVENANCE.md`, header LICENSE section | public-domain dedication with fallback permission; retain actual terms. |
| bcdec | `fresh/third_party/bcdec/PROVENANCE.md`, `LICENSE` | project selects upstream MIT option. |
| Unicode CP936 table | `fresh/third_party/unicode-cp936/PROVENANCE.md`, `LICENSE.txt` | Unicode License V3. |

## External platform inputs

These are not supplied by this repository's self-contained host build.
CEGUI license texts inspected locally are preserved in
`licenses/external/cegui-0.4.1/` with hashes and evidence limits.

| Dependency | Use | Required release evidence |
| --- | --- | --- |
| CEGUI 0.4.1 | application GUI | inspected COPYING and CEGUISystem.cpp establish LGPL-2.1-or-later, not the MIT license of newer releases; archive exact source/hash and bundled PCRE/TinyXML notices |
| FreeType | font rendering | exact version, chosen FTL/GPL option and notice |
| FFmpeg | optional Android Bink decoding | exact revision, configuration, actual LGPL/GPL build status; static-link source/relink obligations |
| SDL2, SDL2_ttf, SDL2_mixer | Linux/Switch | versions, licenses and transitive codec/font dependencies |
| mpv | Linux/Switch video | actual version/build options; GPL/LGPL variant and linked dependencies |
| libnx/devkitPro portlibs | Switch | exact package versions, dependency licenses and build inputs |
| Android NDK libc++ | Android | NDK revision and runtime redistribution notices |

The user selected GPL-3.0-or-later for project-owned code on 2026-11-03.
miniLZO's GPL-2.0-or-later permits using GPL version 3 for a combined work;
its original notices and source obligations remain. A miniLZO replacement is
not required solely to avoid GPL licensing under the selected project license.
Actual external dependency configurations and provenance of project source
must be resolved first. Relicensing project-owned files does not relicense
third-party code or original-game material.

## Proprietary research inputs and game assets

RenderWare SDK archives, original D3DX inputs and original CEGUI/OIRAMLOOK,
Miles/Bink game DLLs are external inputs to the Windows research path.
They are not covered by any future project license and must not be included
without appropriate redistribution rights. The portable runtime uses separate
implementations; this does not automatically clear the entire research tree.

The project reconstructs behavior with IDA evidence. The historical README
word “clean” must not be read as a claim of independently verified clean-room
development. Original-derived source snippets, byte fixtures, screenshots and
archived private library reconstructions still need provenance review before
publishing the complete repository or its history.
