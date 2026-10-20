# ⚡ AY Zygisk V2 — Black & Gold Edition

**Developer:** Eng. Ahmed Yasir  
**Module ID:** `ayzygisk`  
**Release:** `2.0.0` (controller-only; native engine pending)  
**Type:** Independent control/diagnostic module (not a standalone Zygisk engine)

## Installation

Single ZIP: open **Magisk / KernelSU / APatch → Modules → Install from storage → select ZIP → Reboot**. This installs the AY V2 controller on compatible Android devices. KernelSU/APatch web interfaces require a WebUI-capable manager. Magisk versions without embedded WebUI can use the module action or terminal commands.

### Features
- Black/gold responsive Arabic/English WebUI; lightning logo.
- Root manager / device / installed Zygisk engine **marker** inventory (not proof of injection).
- Native ELF/ABI **read-only** preflight; does not load third-party code.
- Compatibility Doctor with honest status classifications.
- SHA-256 Integrity Guard for 10 bundled project files (local check only).
- Monitor boot-time changes of installed modules with a one-shot scan.
- Persistent, local diagnostics and optional size-limited event history.
- Allowlisted settings, saved locally and optionally backed up/restored.
- Optional independent C++ host test environment and Zygisk public API bridge *source*.

### Limitations / accuracy
The installable ZIP does **not** ship a Zygote injector, Android Zygisk `.so` bridge, or a full Zygisk implementation. It cannot replace a standalone Zygisk engine, and Magisk's built-in Zygisk is separate. KernelSU/APatch need an existing supported Zygisk engine to run third-party Zygisk modules. Android NDK and physical device verification are still required. This project does not hide root, modify protected apps, or bypass device integrity checks.

### Command line (Android with root)

```sh
su -c 'sh /data/adb/modules/ayzygisk/scripts/ayctl.sh doctor'
su -c 'sh /data/adb/modules/ayzygisk/scripts/ayctl.sh verify'
su -c 'sh /data/adb/modules/ayzygisk/scripts/ayctl.sh report'
```

Use `action.sh` through the manager for one-click report generation where supported. Logs and reports remain on device under the AY module directory. 

### Packaging and tests

```sh
python3 tools/build.py
sh tests/test-controller.sh
node --check webroot/assets/app.js
python3 native/zygisk/tests/test_bridge.py
python3 tests/test-package.py ../AY-Zygisk-V2-Controller-2.0.0.zip
# Optional host lab (Linux):
cmake -S native -B /tmp/ay-v2-native -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/ay-v2-native --parallel 2
python3 native/tests/test_native.py /tmp/ay-v2-native/ay-native
python3 native/runtime/tests/test_runtime.py /tmp/ay-v2-native/ay-runtime-host /tmp/ay-v2-native
```

For optional ABI-v5 bridge cross-compilation use a real Android NDK and the official `zygisk.hpp`; see `native/zygisk/README.md`. A bridge needs a working external Zygisk backend and does not implement a new engine.

### Publishing

Maintainer: Eng. Ahmed Yasir, site: https://www.magiskmodule.com/author/ahmedyasir2023 . Do not claim on-device verification until Android installs, upgrades, and uninstalls have been tested across all three managers.
