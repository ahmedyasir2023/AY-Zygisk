<div align="center">

<img src="branding/ay-zygisk-banner.svg" alt="AY Zygisk V3.0.0 — Black & Gold by Eng. Ahmed Yasir" width="100%" />

# AY Zygisk V3.0.0

**BLACK & GOLD EDITION**

**Designed & integrated by Eng. Ahmed Yasir**

![Version](https://img.shields.io/badge/Version-3.0.0-E5B968?style=flat-square&labelColor=0D0B08)
![Platform](https://img.shields.io/badge/Android-Root%20Module-E5B968?style=flat-square&labelColor=0D0B08)
![UI](https://img.shields.io/badge/WebUI-Black%20%26%20Gold-E5B968?style=flat-square&labelColor=0D0B08)
![Status](https://img.shields.io/badge/Status-Device%20Testing%20Pending-E5B968?style=flat-square&labelColor=0D0B08)

</div>

## About this edition

AY Zygisk V3.0.0 is a refreshed Android root-module package with a consistent
black-and-gold identity, responsive dashboard, live Zygote/runtime monitoring,
module inventory, settings and diagnostic logs.

Unlike the separate independent-engine research branch, this edition **integrates
a verified existing Zygote runtime**. Rebranding does not make it an original
native injection implementation. The native binaries remain byte-for-byte
unchanged to protect their known runtime ABI.

## What's changed in V3

- AY Zygisk V3.0.0 branding in the WebUI, installer messages and module metadata.
- Prominent **Eng. Ahmed Yasir** designer credit in the dashboard and banner.
- Black, bronze and gold visual tokens with distinct light/dark/AMOLED contrast levels.
- Improved keyboard focus visibility and responsive header sizing.
- Pinned binary checksum checks, ZIP integrity checks and license-preservation checks.
- Existing engine installation logic, architecture-specific binaries and
  Zygote service controls preserved.

## Download and installation

1. Open [GitHub Actions](https://github.com/ahmedyasir2023/AY-Zygisk/actions/workflows/ay-v3-black-gold.yml).
2. Select the successful **AY Zygisk V3.0.0 — Black & Gold** run.
3. Download the **AY-Zygisk-V3.0.0-BlackGold** build artifact and extract the inner installable ZIP.
4. Back up your device and ensure another Zygisk injector is not active.
5. Install the ZIP in a supported root manager, reboot, then check **runtime and Zygote status** in the WebUI.

> [!WARNING]
> A successful ZIP verification is **not** proof of working injection.
> Android 14+ / KernelSU Next behavior has not been tested on a physical device
> in this workflow. If the runtime stays inactive, preserve boot and daemon logs.

## Source and compliance

The WebUI theme and AY branding are the V3 changes. The included native runtime
is a derivative integration covered by AGPL/GPL obligations and third-party
component notices. The release archive includes **LICENSE** and **NOTICE.md**
with pinned corresponding source information. Those legal notices are kept
separate from the AY-facing UI and cannot be erased by a branding edit.

## Developer

**Eng. Ahmed Yasir**  
[GitHub: ahmedyasir2023](https://github.com/ahmedyasir2023)
