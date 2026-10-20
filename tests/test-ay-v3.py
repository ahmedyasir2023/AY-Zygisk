#!/usr/bin/env python3
"""Offline release assertions for the AY Zygisk V3 installer archive."""

import hashlib
import sys
from pathlib import Path
from zipfile import ZipFile


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def test(original_path: Path, output_path: Path) -> None:
    with ZipFile(original_path) as original, ZipFile(output_path) as output:
        assert output.testzip() is None
        names = output.namelist()
        assert len(names) == len(set(names))
        assert b"name=AY Zygisk | Black & Gold" in output.read("module.prop")
        assert b"version=v3.0.0" in output.read("module.prop")
        assert b"author=Eng. Ahmed Yasir" in output.read("module.prop")
        assert b"id=onyxzygisk" in output.read("module.prop"), "Keep compatible runtime ID"
        assert b"SKIPUNZIP=1" in output.read("customize.sh")
        assert b"'webroot/*'" in output.read("customize.sh")
        assert b"AY Zygisk V3.0.0" in output.read("webroot/index.html")
        assert b"ay-v3.css" in output.read("webroot/index.html")
        assert b"Eng. Ahmed Yasir" in output.read("webroot/index.html")
        assert b"--color-onx-accent: #eac16e" in output.read("webroot/assets/ay-v3.css")
        assert b"GNU AFFERO GENERAL PUBLIC LICENSE" in output.read("LICENSE")
        assert b"NeoZygisk" in output.read("NOTICE.md")
        assert b"CSOLoader" in output.read("NOTICE.md")
        assert output.read("banner.png").startswith(b"\x89PNG\r\n\x1a\n")
        for abi in ("arm64-v8a", "armeabi-v7a", "x86", "x86_64"):
            for name in (f"bin/{abi}/zygiskd", f"lib/{abi}/libzygisk.so", f"lib/{abi}/libzygisk_ptrace.so"):
                assert output.read(name) == original.read(name), f"Native binary modified: {name}"
        assert output.read("sepolicy.rule") == original.read("sepolicy.rule")
        for name in names:
            if name.endswith(".sha256"):
                assert output.read(name).strip().decode("ascii") == sha(output.read(name[:-7])), name
        for name in ("README.md", "module.prop", "webroot/index.html", "webroot/config.json"):
            assert b"OnyxZygisk" not in output.read(name), name
        for name in names:
            if name.startswith("webroot/assets/") and name.endswith(".js"):
                assert b"OnyxZygisk" not in output.read(name), name

    print(f"PASS AY Zygisk V3 archive: {output_path} ({sha(output_path.read_bytes())})")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Usage: test-ay-v3.py PINNED_UPSTREAM_ZIP AY_V3_ZIP")
    test(Path(sys.argv[1]), Path(sys.argv[2]))
