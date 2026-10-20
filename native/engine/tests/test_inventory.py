#!/usr/bin/env python3
"""Read-only module discovery tests; no Android injection or root required."""
from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile


ARCH = {
    "arm64-v8a": (2, 183),
    "armeabi-v7a": (1, 40),
    "x86": (1, 3),
    "x86_64": (2, 62),
}


def elf(abi: str, kind: int = 3) -> bytes:
    cls, machine = ARCH[abi]
    out = bytearray(128)
    out[:4] = b"\x7fELF"
    out[4:7] = bytes([cls, 1, 1])
    out[16:18] = kind.to_bytes(2, "little")
    out[18:20] = machine.to_bytes(2, "little")
    out[20:24] = (1).to_bytes(4, "little")
    return bytes(out)


def add(root: Path, name: str, abi: str = "arm64-v8a", *, kind: int = 3) -> Path:
    module = root / name
    (module / "zygisk").mkdir(parents=True)
    (module / "module.prop").write_text(f"id={name}\n", encoding="utf-8")
    (module / "zygisk" / f"{abi}.so").write_bytes(elf(abi, kind))
    return module


def scan(exe: Path, root: Path, abi: str) -> list[str]:
    run = subprocess.run(
        [str(exe), str(root), abi],
        check=True,
        capture_output=True,
        text=True,
        timeout=10,
    )
    return run.stdout.splitlines()


def main(executable: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="ay-engine-inventory-") as temp:
        base = Path(temp)
        root = base / "modules"
        root.mkdir()
        add(root, "healthy")
        add(root, "disabled").joinpath("disable").touch()
        add(root, "removed").joinpath("remove").touch()
        add(root, "wrong_arch", "x86")
        add(root, "wrong_kind", kind=2)
        add(root, "illegal\nname")
        dual = add(root, "dual")
        (dual / "zygisk" / "x86_64.so").write_bytes(elf("x86_64"))

        # Symlinks are not allowed at the root, module, zygisk or ELF level.
        (root / "module_link").symlink_to(root / "healthy", target_is_directory=True)
        (root / "symlink_so").mkdir()
        (root / "symlink_so" / "module.prop").write_text("id=symlink_so\n")
        (root / "symlink_so" / "zygisk").mkdir()
        (root / "symlink_so" / "zygisk" / "arm64-v8a.so").symlink_to(
            root / "healthy" / "zygisk" / "arm64-v8a.so"
        )
        (root / "symlink_dir").mkdir()
        (root / "symlink_dir" / "module.prop").write_text("id=symlink_dir\n")
        (root / "symlink_dir" / "zygisk").symlink_to(
            root / "healthy" / "zygisk", target_is_directory=True
        )
        actual = scan(executable, root, "arm64-v8a")
        expected = ["CANDIDATE|dual|arm64-v8a|128", "CANDIDATE|healthy|arm64-v8a|128"]
        assert actual == expected, (actual, expected)
        assert scan(executable, root, "x86_64") == ["CANDIDATE|dual|x86_64|128"]
        assert scan(executable, root, "not-an-abi") == []
        assert scan(executable, root / "missing", "arm64-v8a") == []
        (base / "root_link").symlink_to(root, target_is_directory=True)
        assert scan(executable, base / "root_link", "arm64-v8a") == []
    print("PASS: original AY native module inventory security cases")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: test_inventory.py EXECUTABLE")
    main(Path(sys.argv[1]).resolve())
