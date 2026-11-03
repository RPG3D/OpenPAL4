#!/usr/bin/env python3
"""Local Git path/selected-secret inventory; never establishes release clearance."""
import argparse
import collections
import json
import re
import subprocess
from pathlib import Path

RISKY = {'.exe', '.dll', '.lib', '.obj', '.pdb', '.cpk', '.csb', '.bik',
         '.dff', '.bsp', '.dds', '.ttf', '.apk', '.nro', '.zip', '.7z',
         '.jks', '.keystore', '.key', '.pem', '.idb', '.i64'}
PATTERNS = {
    'private-key': re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----'),
    'github-token': re.compile(rb'(?:gh[pousr]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{60,})'),
    'aws-access-id': re.compile(rb'AKIA[A-Z0-9]{16}'),
    'openai-token': re.compile(rb'sk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{40,}'),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    def git(*parts):
        return subprocess.check_output(['git', *parts], cwd=root)
    files = git('ls-files', '-z').decode().split('\0')[:-1]
    history = []
    for row in git('rev-list', '--objects', '--all').decode(errors='replace').splitlines():
        oid, _, name = row.partition(' ')
        if name and (Path(name).suffix.lower() in RISKY or name.startswith('refcode/')):
            history.append({'object': oid, 'path': name})
    hits, counts, total = [], collections.Counter(), 0
    # Includes unreachable objects in the local object store. Matched contents
    # are never emitted: only object identifiers and pattern category names.
    proc = subprocess.Popen(['git', 'cat-file', '--batch-all-objects', '--batch'],
                            cwd=root, stdout=subprocess.PIPE)
    try:
        while header := proc.stdout.readline():
            oid, kind, size = header.decode().split()
            size = int(size)
            data = proc.stdout.read(size)
            if len(data) != size or proc.stdout.read(1) != b'\n':
                raise RuntimeError('incomplete cat-file object stream')
            counts[kind] += 1
            total += size
            if kind == 'blob':
                for label, pattern in PATTERNS.items():
                    if pattern.search(data):
                        hits.append({'object': oid, 'kind': label})
        if proc.wait() != 0:
            raise RuntimeError('git cat-file failed')
    finally:
        proc.stdout.close()
        if proc.poll() is None:
            proc.terminate()
            proc.wait()
    report = {
        'head': git('rev-parse', 'HEAD').decode().strip(),
        'tracked_files': len(files),
        'tracked_risky_paths': [p for p in files if Path(p).suffix.lower() in RISKY],
        'history_reachable_risky_paths': history,
        'scanned_object_counts': dict(counts), 'scanned_bytes': total,
        'high_confidence_secret_candidates': hits,
        'limits': ['Selected patterns cannot prove absence of secrets or copyrighted assets.',
                   'Paths cover all refs; selected secret patterns cover the local object store.',
                   'Untracked and modified working-file contents are not scanned by this Git-object audit.',
                   'No history rewritten; no uploads performed.'],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf8')
    print(json.dumps({'tracked_files': len(files), 'risky_current': len(report['tracked_risky_paths']),
                      'risky_history': len(history), 'objects': dict(counts), 'bytes': total,
                      'secret_candidate_count': len(hits)}))


if __name__ == '__main__':
    main()
