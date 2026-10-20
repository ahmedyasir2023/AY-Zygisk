# AY Zygisk V2 – 2.0.0

- New independent V2 controller release with stable id `ayzygisk` and black/gold UI.
- Device readiness doctor with explicit pass/warn/unknown/not-implemented states.
- Integrity Guard verifies release SHA-256 checksums for 10 critical AY code/UI files.
- Whitelisted, local-only profile backup/restore of the three AY settings.
- Enhanced V2 bilingual interface with live Doctor, Integrity and backup controls.
- Preserves native ELF preflight, compatibility inventory, rotation of AY logs and Linux runtime lab.
- Installer avoids touching third-party modules or installing unverified Android/Zygote libraries.

**Scope:** V2 controller & compatibility diagnostics. No independent Zygote injector/engine, bypassing root detection, or production-verified Android native binaries are included.
