#!/usr/bin/env python3
"""Create a review-only portable host source snapshot, never a cleared release."""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PREFIXES = ('fresh/include/', 'fresh/src/', 'fresh/tests/',
            'fresh/ports/runtime/', 'fresh/ports/tools/free_movement_probe/',
            'fresh/native/angelscript/', 'fresh/third_party/')
ROOT_FILES = {'README.md', 'README.en.md', 'BUILDING.md', 'THIRD_PARTY_NOTICES.md', '.gitignore',
              'LICENSE', 'LICENSING.md'}
EXCLUDED = {'.exe', '.dll', '.lib', '.a', '.obj', '.pdb', '.cpk', '.csb', '.bik',
            '.dff', '.bsp', '.dds', '.ttf', '.png', '.jpg', '.jpeg', '.zip', '.7z',
            '.apk', '.nro', '.key', '.pem', '.jks', '.keystore'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--committed-path', action='append', default=[],
                        help='export HEAD bytes for a selected path, excluding its unrelated local edits')
    args = parser.parse_args()
    output = args.output.resolve()
    if output == ROOT or ROOT in output.parents:
        parser.error('output must be outside the checkout')
    if output.exists():
        parser.error('output must be a new directory; existing files are never overwritten')
    tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode().split('\0')
    notes = {'docs/release/STATUS.md', 'docs/release/ANDROID_PUBLIC_BUILD.md',
             '.github/workflows/portable-host.yml',
             'fresh/ports/tools/export_host_source.py',
             'fresh/ports/tools/audit_source_repository.py',
             'fresh/ports/tools/audit_release.py', 'fresh/ports/tools/test_audit_release.py'}
    notes.update(p.relative_to(ROOT).as_posix() for p in (ROOT / 'licenses').rglob('*') if p.is_file())
    paths = sorted(set(p for p in tracked if p and p.startswith(PREFIXES)) | ROOT_FILES | notes)
    if any(p not in paths or p not in tracked for p in args.committed_path):
        parser.error('--committed-path must name a tracked file included in this snapshot')
    output.mkdir(parents=True)
    records, excluded = [], []
    for name in paths:
        source = ROOT / name
        if not source.is_file() or source.is_symlink():
            raise RuntimeError(f'missing, non-file or symlink input: {name}')
        if source.suffix.lower() in EXCLUDED:
            excluded.append(name)
            continue
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if name in args.committed_path:
            target.write_bytes(subprocess.check_output(['git', 'show', f'HEAD:{name}'], cwd=ROOT))
        else:
            shutil.copyfile(source, target)
        records.append({'path': name, 'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
                        'bytes': target.stat().st_size})
    # A host-only snapshot must not advertise absent application/research files.
    (output / 'README.en.md').write_text(
        '# PAL4 portable host source audit candidate\n\n'
        '[简体中文](README.md) | **English**\n\n'
        'See [BUILDING.md](BUILDING.md) for the C++20 host contract build and '
        '[release status](docs/release/STATUS.md) for unresolved gates.\n\n'
        'This snapshot includes portable runtime sources/tests and their vendored '
        'dependencies. Platform application projects, proprietary SDKs, research '
        'archives, screenshots and Git history are excluded. It is not a full '
        'game or a cleared public release. Project-owned code uses '
        '[GPL-3.0-or-later](LICENSING.md); third-party terms remain applicable.\n\n'
        'SOURCE_MANIFEST.json records exact files and excluded local changes.\n', encoding='utf8')
    for record in records:
        if record['path'] == 'README.en.md':
            data = (output / 'README.en.md').read_bytes()
            record.update(sha256=hashlib.sha256(data).hexdigest(), bytes=len(data), generated=True)
    manifest = {'scope': 'portable host audit candidate, no publication clearance',
                'source_head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT).decode().strip(),
                'working_files': True, 'license_selected': True,
                'project_license': 'GPL-3.0-or-later',
                'committed_paths': args.committed_path,
                'limits': ['Original-derived source and fixtures still require provenance review.',
                           'Paths/hashes do not prove legal ownership or absence of embedded assets.',
                           'Platform application projects are outside this host-only snapshot.'],
                'excluded': excluded, 'files': records}
    (output / 'SOURCE_MANIFEST.json').write_text(json.dumps(manifest, indent=2), encoding='utf8')
    print(json.dumps({'output': str(output), 'files': len(records), 'excluded': len(excluded)}))


if __name__ == '__main__':
    main()
