#!/usr/bin/env python3
"""Executable integration tests for the AY read-only native preflight component."""
import pathlib
import subprocess
import sys
import tempfile

native = pathlib.Path(sys.argv[1]).resolve()
tests = pathlib.Path(__file__).parent
with tempfile.TemporaryDirectory() as tmp:
    tmp = pathlib.Path(tmp)
    demo = tmp / 'arm64-v8a.so'
    cc = ['c++', '-shared', '-fPIC', '-o']
    subprocess.run(cc+[str(demo), str(tests/'demo.cpp')], check=True)
    valid = subprocess.run([str(native), 'inspect', str(demo)], text=True, capture_output=True)
    assert valid.returncode == 0 and '|valid-zygisk|x86_64|module|' in valid.stdout, valid.stdout
    noentry=tmp/'noentry.so'
    subprocess.run(cc+[str(noentry), str(tests/'no_entry.cpp')], check=True)
    missing=subprocess.run([str(native), 'inspect', str(noentry)], text=True, capture_output=True)
    assert missing.returncode==1 and '|missing-module-entry|' in missing.stdout, missing.stdout
    invalid=tmp/'invalid.so'
    invalid.write_bytes(b'NOT AN ELF\x00'+b'\x00'*64)
    bad=subprocess.run([str(native), 'inspect', str(invalid)], text=True, capture_output=True)
    assert bad.returncode==1 and '|invalid-elf|' in bad.stdout, bad.stdout
    root=tmp/'modules'
    libpath=root/'testmod'/'zygisk'
    libpath.mkdir(parents=True)
    (root/'testmod'/'module.prop').write_text('id=testmod\nname=Test Mod\n')
    (libpath/'x86_64.so').write_bytes(demo.read_bytes())
    out=subprocess.run([str(native), 'inventory', str(root)], text=True, capture_output=True)
    assert out.returncode==0 and '|valid-zygisk|' in out.stdout and 'SUMMARY|native_libraries|1' in out.stdout, out.stdout
    (libpath/'arm64-v8a.so').write_bytes(demo.read_bytes())
    out=subprocess.run([str(native), 'inventory', str(root)], text=True, capture_output=True)
    assert out.returncode==1 and '|architecture-mismatch|' in out.stdout, out.stdout
    oversize=tmp/'large.so'
    with oversize.open('wb') as f:
        f.truncate(64*1024*1024+1)
    out=subprocess.run([str(native), 'inspect', str(oversize)], text=True, capture_output=True)
    assert out.returncode==1 and '|too-large|' in out.stdout, out.stdout
    print('PASS: AY Zygisk native ELF parser + ABI + exported entry + read-only inventory')
