#!/usr/bin/env python3
"""Check install ZIP safety and reproducibility contracts (host-only)."""
from __future__ import annotations

import hashlib
from pathlib import Path
import sys
from zipfile import ZipFile


CRITICAL = {
    "module.prop",
    "customize.sh",
    "action.sh",
    "service.sh",
    "uninstall.sh",
    "scripts/ayctl.sh",
    "webroot/index.html",
    "webroot/assets/app.css",
    "webroot/assets/app.js",
    "webroot/assets/mark.svg",
}
DEFAULT_NATIVE_MARKER = "standalone-not-implemented"


def check(package: Path) -> None:
    with ZipFile(package) as archive:
        members = archive.namelist()
        assert len(members) == len(set(members)), "Duplicate package entries"
        assert all(
            name and not name.startswith("/") and ".." not in Path(name).parts
            for name in members
        ), "Unsafe archive entry"
        assert CRITICAL <= set(members), "Missing important files"
        assert "integrity.sha256" in members, "Missing integrity manifest"
        manifest = archive.read("integrity.sha256").decode("ascii")
        actual = {}
        for entry in manifest.splitlines():
            digest, sep, path = entry.partition("  ")
            assert sep and len(digest) == 64 and path in CRITICAL
            assert path not in actual, f"Duplicate integrity entry: {path}"
            actual[path] = digest
            assert digest == hashlib.sha256(archive.read(path)).hexdigest(), path
        assert set(actual) == CRITICAL, "Incomplete integrity manifest"
        props = archive.read("module.prop").decode("utf-8")
        assert "id=ayzygisk" in props.splitlines()
        assert "version=2.0.0" in props.splitlines()
        backend = archive.read("scripts/ayctl.sh").decode("utf-8")
        assert DEFAULT_NATIVE_MARKER in backend, "Unexpected backend status claim"
        # The controller build must not silently bundle Zygote injection code.
        assert not any(name.startswith("zygisk/") for name in members), (
            "Standalone Zygisk libraries require separate, explicit integration"
        )
        assert archive.testzip() is None, "Corrupted package member"
    print(f"PASS: controller-only archive {package.name}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: test-package.py ZIPFILE")
    check(Path(sys.argv[1]))
