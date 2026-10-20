# AY Zygisk API v5 Bridge — optional developer build

This is an original, minimal *Zygisk plugin*, not an injector, not a Zygote engine. It exercises `onLoad`, `preAppSpecialize`, and `preServerSpecialize` against the official public Zygisk API, requests clean library unload, and does not hook, hide, or identify applications. It requires an already working Zygisk backend. Do not ship to end users without real-device validation.

## Official header (required, NOT bundled)

Place the **unaltered** `zygisk.hpp` public API header from the official Magisk sample under `native/zygisk/vendor/zygisk.hpp`. Keep the original license header. Source URL:
https://github.com/topjohnwu/zygisk-module-sample/blob/master/module/jni/zygisk.hpp

The header is published with a permissive ISC-style license by topjohnwu. Do not replace it with the **mock** test header in `tests/include`; the mock cannot produce an Android ABI-compatible plugin.

## Compile Android shared objects

```sh
python3 tools/build-zygisk.py --ndk "$ANDROID_NDK_HOME" --abi all
python3 tools/build.py --with-zygisk
```

The build stages `zygisk/arm64-v8a.so`, `zygisk/armeabi-v7a.so`, `zygisk/x86.so`, and `zygisk/x86_64.so` if all four succeed. `--with-zygisk` is opt-in and rejects missing or wrong-architecture ELF files. The root installer does not enable Zygisk itself. Some devices/backends only use one or two ABIs.

## Host-only callback contract tests

```sh
python3 native/zygisk/tests/test_bridge.py
```

These tests only confirm callback logic using a mock. They **cannot** validate Zygote integration, Zygisk ABI layout, or device compatibility.
