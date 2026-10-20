<div align="center">

![AY Zygisk — Black & Gold Banner](https://raw.githubusercontent.com/ahmedyasir2023/AY-Zygisk/feat/independent-engine-foundation/branding/ay-zygisk-banner.svg)

# AY Zygisk V2.0.0 RC1 — Black & Gold

**Android 14+ readiness • KernelSU Next diagnostics • Four-ABI native build**

**إصدار تجريبي من لوحة تحكم AY Zygisk وفحوصات توافق KernelSU Next**

</div>

## Highlights | الجديد

- Black & Gold bilingual WebUI with module controls and diagnostics.
- Android 14+ / KernelSU Next readiness checker (observable status, not runtime proof).
- Native Android ELF/ABI inventory, built for `arm64-v8a`, `armeabi-v7a`, `x86` and `x86_64`.
- Integrity manifest, read-only compatibility doctor and diagnostic report commands.
- Original native engine **foundation** (control-plane lifecycle and recovery guard); independent runtime loading is still a roadmap item.
- Updated README and project banner shipped with the controller ZIP.

## Download & installation | التنزيل والتثبيت

The CI artifact contains the installable **`AY-Zygisk-V2-Controller-2.0.0.zip`** inside an outer ZIP, alongside the individual native ABI binaries.

1. Download the [verified Android NDK build artifact](https://github.com/ahmedyasir2023/AY-Zygisk/actions/runs/39351320040/artifacts/12233028271).
2. Unzip the **outer** build artifact on your computer or phone.
3. Locate `AY-Zygisk-V2-Controller-2.0.0.zip`. **Do not extract that inner ZIP.**
4. Install that inner ZIP from **KernelSU Next → Modules** and reboot **on a non-primary test device only**.
5. Use the WebUI or run diagnostics as root:
   ```sh
   su -c 'sh /data/adb/modules/ayzygisk/scripts/ayctl.sh status'
   su -c 'sh /data/adb/modules/ayzygisk/scripts/ayctl.sh ksun'
   su -c 'sh /data/adb/modules/ayzygisk/scripts/ayctl.sh doctor'
   su -c 'sh /data/adb/modules/ayzygisk/scripts/ayctl.sh verify'
   ```

**Checksum of CI outer artifact:** `SHA-256 21dce4bcfd25e043ba1b384d62ddf53be6214ab47762511cf4466820fa1d2e2c` (GitHub Actions artifact archive only).

## Verification | التحقق

- ✅ Android NDK build completed successfully on all four configured ABIs.
- ✅ Controller package generated and uploaded by GitHub Actions.
- ✅ Desktop regression tests available in CI.
- ⏳ Independent Zygote attachment and runtime-loading integration: not implemented.
- ⏳ Physical-device boot, SELinux and crash-recovery testing: pending.

Build run: https://github.com/ahmedyasir2023/AY-Zygisk/actions/runs/39351320040  
Source commit: [`89c0520`](https://github.com/ahmedyasir2023/AY-Zygisk/commit/89c05202a862d0418a7d620eb6c15fa1c015142b)  
Development source: https://github.com/ahmedyasir2023/AY-Zygisk/tree/feat/independent-engine-foundation

## Maintainer

**Eng. Ahmed Yasir** · [@ahmedyasir2023](https://github.com/ahmedyasir2023)

**Status:** Experimental prerelease; use at your own risk on a recoverable test device.
