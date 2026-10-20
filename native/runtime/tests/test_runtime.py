#!/usr/bin/env python3
"""Host-only integration test: callback lifecycle, rejected ABI, fault isolation, timeout."""
import pathlib
import subprocess
import sys
exe, libdir = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
cases = [
    ('ay-test-ok', 0, 'OK|ay-fixture|prepare|event-1-ok|complete|event-2-ok'),
    ('ay-test-wrong', 3, 'FAIL|unsupported-ay-abi'),
    ('ay-test-crash', 4, 'FAIL|plugin-crashed'),
    ('ay-test-hang', 5, 'FAIL|timeout-or-wait-error'),
]
for lib, code, expected in cases:
    path = libdir / (lib + '.so')
    done = subprocess.run([str(exe), 'test-plugin', str(path)], text=True, capture_output=True, timeout=7)
    assert (done.returncode == code and expected in done.stdout), (lib, done.returncode, done.stdout, done.stderr)
    print('PASS', lib, done.returncode)
print('PASS: four AY runtime host lab integration cases')
