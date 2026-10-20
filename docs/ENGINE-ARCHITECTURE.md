# AY Zygisk — Independent Native Engine Architecture

## Verified baseline

The V2 release provides a controller, WebUI, diagnostics, ELF static inspection,
a Linux-only plugin test harness and a public Zygisk API-v5 **module-side**
bridge. None of these loads code into Android Zygote. The status key
`standalone-not-implemented` remains the truthful default.

## Safety requirements for native development

1. Use original, auditable code. Do not reuse proprietary Zygisk Next binaries.
2. Keep the root daemon separate from process-resident code and use authenticated
   local IPC with a strict request schema.
3. Track 32-bit and 64-bit Zygote lifecycles independently.
4. Do not load unvalidated paths or mismatch ABI variants.
5. Keep experimental injection explicitly disabled by default.
6. Stop repeated startup failures before they can trigger a boot loop.
7. Exclude root concealment, bypasses and integrity evasion from scope.

## Engineering gates

| Milestone | Required implementation | Acceptance evidence |
| --- | --- | --- |
| M1 source baseline | Controller, CI, manifest test | Build logs, passing package tests |
| M2 native engine | Original Zygote attachment, process bridge and root companion | NDK builds and controlled device boot logs |
| M3 API compatibility | v5 pre/post callbacks, module dir, companion, FD and hook contracts | Public API conformance tests on Android |
| M4 recovery | Shutdown, disable and rollback paths | Reboot/crash-loop recovery on test devices |
| M5 qualification | Manager/Android/ABI/SELinux/module matrix | Physical device test results |

## Current status

- **M1:** Host test and four-ABI NDK build pipelines pass; physical-device installation and boot behavior remain unverified.
- **M2–M5:** Not complete. Do not publish a standalone-engine release.

### M2 control-plane foundation (source implemented, host-tested)

- `native/engine/core.hpp` and `core.cpp` define the native engine state
  machine with failure thresholds and explicit disable/reset semantics.
- The binary IPC envelope is length-bounded, versioned and rejects unknown
  command types; unit tests exercise malformed headers and 100,000 random frames.
- A read-only, descriptor-relative inventory scanner rejects symlinks, disabled
  modules, unexpected ELF classes and ABI mismatches; it never executes a module.
- `native/engine/recovery.*` implements an original persistent crash-loop guard using
  a process lock, atomic journal replacement, durability sync and fail-closed
  corruption and symlink checks. Host tests cover three unclean starts, operator
  reset and on-disk state errors. It is not yet wired to an Android root daemon.
- No socket is listening, no root privileges are used, and there is no Zygote
  attachment. Peer authentication and device-verified recovery remain required.

The bridge in `native/zygisk` is a plugin that requires an external backend.
It cannot serve as the standalone loader. The host harness uses an AY-specific
ABI and must not be presented as Zygisk API compatibility.

For M2, the backend must be implemented and built specifically for Android.
Android's Zygote may spawn both primary and secondary ABI processes; WebView
Zygote and USAP behavior also require coverage. Runtime tests on a desktop
cannot establish compatibility, reliability or system startup safety.

## Selected Android target: Android 14+ with KernelSU Next

- Treat Android SDK 34 as the minimum for the **future independent engine**;
  the legacy V2 read-only controller can still run on older supported Android.
- KernelSU Next officially supports primary `arm64-v8a` and `x86_64`
  architectures. A 64-bit device may still use a separate 32-bit Zygote;
  both process lifecycles require independent validation.
- KernelSU's documented `KSU_LATE_LOAD=1` mode skips the normal early boot
  hooks and is **not eligible for assumptions about pre-Zygote attachment**.
- This module has `skip_mount` and no system directory. A metamodule is
  unnecessary for our read-only diagnostics; do not install another one just
  for AY.
- `scripts/ayctl.sh ksun` provides read-only compatibility observations.
  A generic KernelSU family marker does **not** cryptographically prove the
  KernelSU Next variant or guarantee backend functionality.
- Physical test requirements: exact device/kernel, SELinux mode, standard vs
  late-load, boot logs, current Zygisk modules and a known recovery route.
- Until tested, independent injection remains disabled and unreleased.

Primary reference: [KernelSU Next official metamodule guide](https://github.com/KernelSU-Next/KernelSU-Next/blob/dev/website/docs/guide/metamodule.md)
