#!/usr/bin/env python3
"""Package AY Zygisk V3 with a new visual identity and an unchanged Zygote engine.

This is a branded integration of a pinned AGPL-3.0 implementation, not a
source-level rename of its module ID.  The private work directory and binary
contracts must not be changed until the native runtime is rebuilt together.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from copy import copy
from pathlib import Path, PurePosixPath
from xml.etree import ElementTree
from zipfile import ZIP_DEFLATED, ZipFile


SOURCE_DIGEST = "76fc0ed44186a628d3816a2830320e234ed62e1aa962be99bc1bf224f5505de7"
ABIS = {
    "arm64-v8a": (2, 183),
    "armeabi-v7a": (1, 40),
    "x86": (1, 3),
    "x86_64": (2, 62),
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def checked_replace(data: bytes, old: bytes, new: bytes, file: str) -> bytes:
    if old not in data:
        raise ValueError(f"Expected branding string missing in {file}: {old!r}")
    return data.replace(old, new)


def validate_archive(source: ZipFile) -> None:
    entries = source.infolist()
    names = {entry.filename for entry in entries}
    if len(names) != len(entries) or source.testzip() is not None:
        raise ValueError("Input ZIP has duplicate filenames or failed CRC")
    for entry in entries:
        path = PurePosixPath(entry.filename)
        if path.is_absolute() or ".." in path.parts or "\\" in entry.filename:
            raise ValueError(f"Unsafe archive path: {entry.filename}")
        if (entry.external_attr >> 16) & 0o170000 == 0o120000:
            raise ValueError(f"Archive includes symlink: {entry.filename}")
    required = {
        "module.prop", "customize.sh", "verify.sh", "zygisk-init.sh", "post-fs-data.sh",
        "service.sh", "late-load.sh", "sepolicy.rule", "webroot/index.html",
        "webroot/config.json", "banner.png",
    }
    if required - names:
        raise ValueError(f"Missing source entries: {sorted(required - names)}")
    for abi, architecture in ABIS.items():
        for file in (f"bin/{abi}/zygiskd", f"lib/{abi}/libzygisk.so", f"lib/{abi}/libzygisk_ptrace.so"):
            data = source.read(file)
            if data[:4] != b"\x7fELF" or data[5] != 1:
                raise ValueError(f"Invalid ELF header: {file}")
            observed = (data[4], int.from_bytes(data[18:20], "little"))
            if observed != architecture:
                raise ValueError(f"Unexpected ELF architecture: {file}")
    for name in names:
        if name.endswith(".sha256"):
            payload = name[:-7]
            if payload not in names or source.read(name).strip().decode("ascii") != sha256(source.read(payload)):
                raise ValueError(f"Invalid checksum sidecar: {name}")
    if b"id=onyxzygisk\n" not in source.read("module.prop"):
        raise ValueError("Incompatible upstream module ID")
    if b"SKIPUNZIP=1" not in source.read("customize.sh"):
        raise ValueError("Expected installer extraction method not present")


def new_webui_html(content: bytes) -> bytes:
    content = checked_replace(content, b"<title>OnyxZygisk</title>", b"<title>AY Zygisk V3.0.0</title>", "webroot/index.html")
    content = checked_replace(
        content,
        b"</head>",
        b'\t\t<meta name="theme-color" content="#100d08" />\n'
        b'\t\t<link rel="stylesheet" href="./assets/ay-v3.css" />\n\t</head>',
        "webroot/index.html",
    )
    # The installer banner is now an actual WebUI image as well. Keep designer
    # credit in HTML so it is readable on narrow phone screens, not just in PNG.
    banner = (
        b'<header id="ay-v3-brand" aria-label="AY Zygisk Black and Gold banner">'
        b'<img class="ay-v3-banner" src="./assets/ay-v3-banner.png" '
        b'alt="AY Zygisk V3.0.0 Black and Gold Edition - Designed by Eng. Ahmed Yasir" />'
        b'<div class="ay-v3-ribbon"><span class="ay-v3-mark">AY <b>ZYGISK</b></span>'
        b'<span class="ay-v3-credit">DESIGNED BY <strong>Eng. Ahmed Yasir</strong></span>'
        b'</div></header>'
    )
    return checked_replace(content, b'<div id="app"></div>', banner + b'\n\t\t<div id="app"></div>', "webroot/index.html")


def build_replacements(source: ZipFile, banner_png: bytes, skin_css: bytes, arabic_xml: bytes) -> dict[str, bytes]:
    names = set(source.namelist())
    props = (
        "id=onyxzygisk\n"  # Native compatibility; do not change without recompilation.
        "name=AY Zygisk | Black & Gold\n"
        "version=v3.0.0\n"
        "versionCode=30000\n"
        "author=Eng. Ahmed Yasir\n"
        "description=AY Zygisk V3.0.0 with a refined black-and-gold WebUI, runtime monitor and module diagnostics.\n"
    ).encode("utf-8")
    updates: dict[str, bytes] = {
        "module.prop": props,
        "banner.png": banner_png,
        "webroot/index.html": new_webui_html(source.read("webroot/index.html")),
    }
    english_root = ElementTree.fromstring(source.read("webroot/locales/strings/en.xml"))
    arabic_root = ElementTree.fromstring(arabic_xml)
    english_keys = {node.get("name") for node in english_root.findall("string")}
    arabic_keys = {node.get("name") for node in arabic_root.findall("string")}
    # The single added key labels the private runtime path honestly without
    # displaying a legacy compatibility identifier.
    if arabic_keys != english_keys | {"status_workdir_private"}:
        raise ValueError(f"Arabic locale keys differ: missing={sorted(english_keys - arabic_keys)}; extra={sorted(arabic_keys - english_keys - {'status_workdir_private'})}")
    languages = json.loads(source.read("webroot/locales/languages.json"))
    if "ar" in languages:
        raise ValueError("Arabic is already present in the pinned upstream locale list")
    languages["ar"] = "العربية"
    updates["webroot/locales/languages.json"] = (json.dumps(languages, indent=4, ensure_ascii=False) + "\n").encode("utf-8")
    config = json.loads(source.read("webroot/config.json"))
    config["title"] = "AY Zygisk V3.0.0"
    updates["webroot/config.json"] = (json.dumps(config, indent=4, ensure_ascii=False) + "\n").encode()
    for name in names:
        if name.startswith("webroot/assets/index-") and name.endswith(".js"):
            js = source.read(name)
            js = checked_replace(js, b"OnyxZygisk", b"AY Zygisk", name)
            js = checked_replace(js, b"le=[`Sai`,`Matsuzaka Yuki`]", b"le=[`Eng. Ahmed Yasir`]", name)
            # The actual location remains owned by the native runtime. The
            # status card is a display-only label, not a file-system action.
            js = checked_replace(
                js,
                b"children:t?.keys.workdir||c",
                b"children:N(`status_workdir_private`,`Managed internally`)",
                name,
            )
            updates[name] = js
        elif name.startswith("webroot/locales/strings/") and name.endswith(".xml"):
            xml = source.read(name)
            # Localized caption; visible author list in the WebUI is rebranded.
            xml = re.sub(rb'(<string name="settings_authors">).*?(</string>)', rb'\1Designer\2', xml)
            if name.endswith("en.xml"):
                xml = checked_replace(
                    xml,
                    b"</resources>",
                    b'\t<string name="status_workdir_private">Managed internally</string>\n</resources>',
                    name,
                )
            updates[name] = xml
        elif name in {"action.sh", "customize.sh", "service.sh", "zygisk-init.sh", "post-fs-data.sh", "late-load.sh", "uninstall.sh"}:
            script = source.read(name)
            if b"OnyxZygisk" in script:
                updates[name] = script.replace(b"OnyxZygisk", b"AY Zygisk")
    if not any(key.endswith(".js") for key in updates):
        raise ValueError("Expected WebUI entrypoint not found")
    if not skin_css.strip().startswith(b"/* AY Zygisk V3.0.0"):
        raise ValueError("Unexpected skin CSS")
    return updates


def readme() -> bytes:
    return (
        "# AY Zygisk V3.0.0\n\n"
        "**Black & Gold Edition — interface and integration by Eng. Ahmed Yasir.**\n\n"
        "This build preserves the verified native Zygote runtime and its internal module\n"
        "paths, with an AY-branded WebUI, installation messages, banner and diagnostics.\n"
        "It has not yet been device-tested for Android 14+ / KernelSU Next.\n\n"
        "## Highlights\n\n"
        "- Refreshed responsive black-and-gold dashboard and accessibility focus states.\n"
        "- Visible AY Zygisk V3.0.0 header with prominent designer credit.\n"
        "- In-app premium banner with a compact mobile credit ribbon.\n"
        "- Full Arabic translation and automatic right-to-left interface layout.\n"
        "- Private work directory is described without exposing legacy runtime IDs.\n"
        "- Runtime/daemon, Zygote, installed modules and logs views kept functional.\n"
        "- Verified module payload checksums and unchanged architecture-specific ELFs.\n\n"
        "## Installation and recovery\n\n"
        "Install using a supported root manager (KernelSU, Magisk, or APatch), reboot,\n"
        "and confirm the Zygote runtime state in the dashboard. Do not install beside\n"
        "a second Zygisk injector. Back up the device and retain a recovery method.\n"
        "If the runtime is inactive, collect daemon and installation logs before\n"
        "changing anything else. Do not assume installation equals injection.\n\n"
        "## License and source\n\n"
        "Native runtime is derived from separately licensed AGPL/GPL components.\n"
        "The required license, notices and pinned source reference are distributed\n"
        "with this archive. See LICENSE and NOTICE.md for the original contributors\n"
        "and the complete corresponding source reference.\n"
    ).encode("utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", required=True, type=Path)
    parser.add_argument("--license", required=True, type=Path)
    parser.add_argument("--notice", required=True, type=Path)
    parser.add_argument("--banner-png", required=True, type=Path)
    parser.add_argument("--skin-css", required=True, type=Path)
    parser.add_argument("--arabic-xml", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.upstream.resolve() == args.output.resolve():
        raise ValueError("Output path would overwrite the original runtime")
    if sha256(args.upstream.read_bytes()) != SOURCE_DIGEST:
        raise ValueError("Runtime archive does not match the pinned official build")
    license_text = args.license.read_bytes()
    notice_text = args.notice.read_bytes()
    if b"GNU AFFERO GENERAL PUBLIC LICENSE" not in license_text:
        raise ValueError("AGPL license missing or invalid")
    if b"NeoZygisk" not in notice_text or b"CSOLoader" not in notice_text:
        raise ValueError("Required multi-contributor NOTICE missing")
    banner_png = args.banner_png.read_bytes()
    if banner_png[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Banner must be PNG")
    skin_css = args.skin_css.read_bytes()
    arabic_xml = args.arabic_xml.read_bytes()
    with ZipFile(args.upstream) as original:
        validate_archive(original)
        updates = build_replacements(original, banner_png, skin_css, arabic_xml)
        updates["README.md"] = readme()
        original_names = set(original.namelist())
        extras = {
            "webroot/assets/ay-v3.css": skin_css,
            "webroot/assets/ay-v3.css.sha256": sha256(skin_css).encode("ascii"),
            "webroot/assets/ay-v3-banner.png": banner_png,
            "webroot/assets/ay-v3-banner.png.sha256": sha256(banner_png).encode("ascii"),
            "webroot/locales/strings/ar.xml": arabic_xml,
            "webroot/locales/strings/ar.xml.sha256": sha256(arabic_xml).encode("ascii"),
            "LICENSE": license_text,
            "NOTICE.md": notice_text + (
                "\n## Complete corresponding source\n\n"
                "https://github.com/OnyxZygisk/OnyxZygisk/tree/820b68b31527c41d5decf4a47765167ebe795dc8\n"
            ).encode("utf-8"),
        }
        if original_names & extras.keys():
            raise ValueError("Archive already contains a V3 or license entry")
        for name, payload in list(updates.items()):
            sidecar = f"{name}.sha256"
            if sidecar in original_names:
                updates[sidecar] = sha256(payload).encode("ascii")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with ZipFile(args.output, "w", ZIP_DEFLATED, compresslevel=9) as dest:
            for info in original.infolist():
                # ZipFile.writestr mutates ZipInfo.header_offset; copying it
                # protects the source archive for verification after writing.
                dest.writestr(copy(info), updates.get(info.filename, original.read(info.filename)))
            for name, payload in extras.items():
                dest.writestr(name, payload)
        with ZipFile(args.output) as final:
            validate_archive(final)
            assert final.read("module.prop") == updates["module.prop"]
            assert final.read("webroot/assets/ay-v3.css") == skin_css
            assert final.read("webroot/assets/ay-v3-banner.png") == banner_png
            assert final.read("webroot/locales/strings/ar.xml") == arabic_xml
            for abi in ABIS:
                for name in (f"bin/{abi}/zygiskd", f"lib/{abi}/libzygisk.so", f"lib/{abi}/libzygisk_ptrace.so"):
                    if final.read(name) != original.read(name):
                        raise ValueError(f"Runtime binary changed unexpectedly: {name}")
            visible_paths = ["module.prop", "webroot/config.json", "webroot/index.html", "README.md"]
            visible_paths += [name for name in final.namelist() if name.startswith("webroot/assets/") and name.endswith(".js")]
            for name in visible_paths:
                if b"OnyxZygisk" in final.read(name):
                    raise ValueError(f"Legacy brand still visible in {name}")
    print(f"{args.output} | sha256={sha256(args.output.read_bytes())}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, AssertionError) as exc:
        sys.exit(f"ERROR: {exc}")
