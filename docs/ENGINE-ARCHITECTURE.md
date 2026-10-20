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

- **M1:** In progress; GitHub Actions and host tests are the immediate evidence.
- **M2–M5:** Not complete. Do not publish a standalone-engine release.

The bridge in `native/zygisk` is a plugin that requires an external backend.
It cannot serve as the standalone loader. The host harness uses an AY-specific
ABI and must not be presented as Zygisk API compatibility.

For M2, the backend must be implemented and built specifically for Android.
Android's Zygote may spawn both primary and secondary ABI processes; WebView
Zygote and USAP behavior also require coverage. Runtime tests on a desktop
cannot establish compatibility, reliability or system startup safety.
