#!/usr/bin/env python3
"""Build an explicitly attributed AY/Onyx experimental Android module.

The upstream release is NEVER executed by this script. All runtime binaries and
boot scripts remain byte-for-byte those of the pinned OnyxZygisk release.
Only module.prop and its SHA-256 sidecar are changed. This is not a clean-room
or independent AY Zygote injector.
"""

import argparse
import hashlib
import sys
from pathlib import Path, PurePosixPath
from zipfile import ZipFile, ZIP_DEFLATED


UPSTREAM_SHA256 = "76fc0ed44186a628d3816a2830320e234ed62e1aa962be99bc1bf224f5505de7"
UPSTREAM_REVISION = "820b68b31527c41d5decf4a47765167ebe795dc8"
RUNTIME_FILES = [
    "zygisk-init.sh", "post-fs-data.sh", "late-load.sh", "service.sh",
    "customize.sh", "sepolicy.rule", "verify.sh",
]
ABIS = {
    "arm64-v8a": (2, 183), "armeabi-v7a": (1, 40),
    "x86": (1, 3), "x86_64": (2, 62),
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def check_elf(data: bytes, arch: tuple[int, int], name: str) -> None:
    if len(data) < 64 or data[:4] != b"\x7fELF":
        raise ValueError(f"Invalid Android ELF: {name}")
    got = (data[4], int.from_bytes(data[18:20], "little"))
    if data[5] != 1 or got != arch:
        raise ValueError(f"Architecture mismatch: {name} {got} != {arch}")


def verify_upstream(source: ZipFile) -> None:
    entries = source.infolist()
    names = [e.filename for e in entries]
    if len(names) != len(set(names)):
        raise ValueError("Duplicate names in upstream archive")
    for entry in entries:
        path = PurePosixPath(entry.filename)
        if path.is_absolute() or ".." in path.parts or "\\" in entry.filename:
            raise ValueError(f"Unsafe archive path: {entry.filename}")
        if (entry.external_attr >> 16) & 0o170000 == 0o120000:
            raise ValueError(f"Unexpected symbolic link: {entry.filename}")
    if source.testzip() is not None:
        raise ValueError("Corrupted upstream archive")
    for name in RUNTIME_FILES + ["module.prop"]:
        if name not in names:
            raise ValueError(f"Missing runtime script: {name}")
    for abi, arch in ABIS.items():
        for name in (f"bin/{abi}/zygiskd", f"lib/{abi}/libzygisk.so",
                     f"lib/{abi}/libzygisk_ptrace.so"):
            check_elf(source.read(name), arch, name)
    for name in names:
        if name.endswith(".sha256"):
            payload = name.removesuffix(".sha256")
            if payload not in names:
                raise ValueError(f"Orphan checksum: {name}")
            recorded = source.read(name).decode("ascii").strip().lower()
            if recorded != digest(source.read(payload)):
                raise ValueError(f"Checksum mismatch: {payload}")
    props = source.read("module.prop").decode("utf-8")
    if "id=onyxzygisk\n" not in props or "version=v1.09\n" not in props:
        raise ValueError("Unexpected upstream module identity or version")
    if b"SKIPUNZIP=1" not in source.read("customize.sh"):
        raise ValueError("Unexpected upstream installer behavior")


def main() -> int:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--upstream", required=True, type=Path)
    cli.add_argument("--license", required=True, type=Path)
    cli.add_argument("--notice", required=True, type=Path)
    cli.add_argument("--banner", required=True, type=Path)
    cli.add_argument("--output", required=True, type=Path)
    args = cli.parse_args()

    if args.output.resolve() == args.upstream.resolve():
        raise ValueError("Output must not overwrite the verified upstream ZIP")
    observed = digest(args.upstream.read_bytes())
    if observed != UPSTREAM_SHA256:
        raise ValueError(f"Unrecognized upstream ZIP checksum: {observed}")
    license_text = args.license.read_bytes()
    notice_text = args.notice.read_bytes()
    banner = args.banner.read_bytes()
    if b"GNU AFFERO GENERAL PUBLIC LICENSE" not in license_text:
        raise ValueError("The complete upstream AGPL-3.0 license is required")
    if b"NeoZygisk" not in notice_text or b"CSOLoader" not in notice_text:
        raise ValueError("The pinned upstream NOTICE.md is required")
    if b"<svg" not in banner[:4096]:
        raise ValueError("Expected AY SVG banner")

    props = (
        "id=onyxzygisk\n"
        "name=AY Zygisk | Onyx Runtime Edition\n"
        "version=v1.09-ay.1\n"
        "versionCode=399\n"
        "author=Eng. Ahmed Yasir (integration); Sai, Matsuzaka Yuki (OnyxZygisk)\n"
        "description=Experimental AY branding of OnyxZygisk v1.09 runtime. AGPL-3.0; not an independently written AY injector.\n"
    ).encode("utf-8")
    readme = (
        "# AY Zygisk — Onyx Runtime Edition (experimental)\n\n"
        "![AY Zygisk banner](branding/ay-zygisk-banner.svg)\n\n"
        "**Actual Zygote injection is supplied by OnyxZygisk v1.09.**\n"
        "This is an attributed third-party runtime integration, **not** the unfinished\n"
        "clean-room AY engine. Upstream binaries and boot scripts are unchanged.\n\n"
        f"- Upstream source: https://github.com/OnyxZygisk/OnyxZygisk/tree/{UPSTREAM_REVISION}\n"
        f"- Exact upstream release SHA-256: `{UPSTREAM_SHA256}`\n"
        "- License: GNU AGPL-3.0; see LICENSE and NOTICE.md in this ZIP.\n"
        "- Module ID is deliberately `onyxzygisk`, because the upstream runtime\n"
        "  hard-codes `/data/adb/onyxzygisk` and expects compatible module paths.\n"
        "- Do **not** install alongside a second active Zygisk injector.\n"
        "- Android 14+ / KernelSU Next support is a target, **not device-tested by AY**.\n"
        "- Back up the device and use an accessible recovery method before testing.\n"
        "- The upstream runtime contains optional functions beyond AY's original\n"
        "  scope; those are retained as received and are not represented as AY code.\n"
    ).encode("utf-8")

    replacements = {"module.prop": props, "module.prop.sha256": digest(props).encode("ascii")}
    extras = {
        "README-AY.md": readme,
        "branding/ay-zygisk-banner.svg": banner,
        "LICENSE": license_text,
        "NOTICE.md": notice_text,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(args.upstream, "r") as original, ZipFile(args.output, "w", ZIP_DEFLATED, compresslevel=9) as output:
        verify_upstream(original)
        original_names = set(original.namelist())
        if original_names.intersection(extras):
            raise ValueError("The upstream archive already contains an AY extra name")
        for info in original.infolist():
            output.writestr(info, replacements.get(info.filename, original.read(info.filename)))
        for name, data in extras.items():
            output.writestr(name, data, compress_type=ZIP_DEFLATED)

    with ZipFile(args.output) as generated:
        if generated.testzip() is not None:
            raise ValueError("Resulting ZIP is damaged")
        for info in generated.infolist():
            if info.filename in extras and generated.read(info.filename) != extras[info.filename]:
                raise ValueError(f"Wrong bundled attribution: {info.filename}")
        for name in original_names - set(replacements):
            with ZipFile(args.upstream) as src:
                if generated.read(name) != src.read(name):
                    raise ValueError(f"Upstream file unexpectedly modified: {name}")

    print(f"{args.output} | sha256={digest(args.output.read_bytes())}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError) as exc:
        sys.exit(f"ERROR: {exc}")
