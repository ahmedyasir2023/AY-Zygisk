# AY Zygisk Native Runtime Lab 0.3.0
  
## AY independent-engine core (in progress)

`engine/core.hpp` and `engine/core.cpp` implement an original, portable
state machine and versioned IPC frame validation. `engine/tests/test_core.cpp`
tests state transitions, bounded parsing and invalid inputs. The sources
compile on Linux and with the Android NDK; they are **not** an injector.
No daemon, socket listener or Zygote attachment is enabled in install ZIPs.

The native tree holds two **different** programs:

1. `src/main.cpp` → `ay-native`: read-only ELF/ABI/symbol scanner for Zygisk `.so` files. Builds on Linux and with Android NDK on Android, without executing inspected code.
2. `runtime/host_runtime.cpp` → `ay-runtime-host`: Linux/desktop-only dynamic plugin **testing** program. Compiled only when `NOT ANDROID`. Loads **AY test plugins**, not Zygisk plugins. The API in `runtime/ay_runtime_api.h` is **not** a drop-in replacement for Magisk's public Zygisk API.

## Desktop test

```sh
cmake -S native -B /tmp/ay-runtime-build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/ay-runtime-build
python3 native/tests/test_native.py /tmp/ay-runtime-build/ay-native
# Run as a NON-root account:
python3 native/runtime/tests/test_runtime.py /tmp/ay-runtime-build/ay-runtime-host /tmp/ay-runtime-build
```

This test harness forks a worker, imposes CPU/memory limits and a 2.5-second deadline, and reports crashes and unsupported AY ABI versions; it is **not a hardened sandbox** for untrusted code. Run only known source-code fixtures. Never use this loader with root privileges.

## Android build

```sh
python3 tools/build-android.py --ndk /path/to/android-ndk --abi all
```

Cross-compiled Android executable is the **ELF static preflight scanner only**. The runtime host loader is deliberately not compiled on Android or included in Magisk/KernelSU/APatch installation ZIPs. This environment has no Android NDK and cannot assert cross-compilation passed.

## Public Zygisk API note

Published Zygisk API v5 is documented at https://github.com/topjohnwu/zygisk-module-sample/blob/master/module/jni/zygisk.hpp . Proper callbacks must be executed within a correctly initialized Zygote-derived process. The AY test protocol is not compatible with those callback signatures and does not claim to be.
