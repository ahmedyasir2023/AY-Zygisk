<div align="center">

![AY Zygisk V3.0.0 — Black & Gold](https://raw.githubusercontent.com/ahmedyasir2023/AY-Zygisk/feat/ay-zygisk-v3-black-gold/branding/ay-zygisk-banner.svg)

# AY Zygisk V3.0.0

**BLACK & GOLD EDITION**

**Designed & integrated by Eng. Ahmed Yasir**

</div>

---

### What's new / الجديد

- **Refined Black & Gold WebUI** — dark surfaces, warm metallic accents, and a responsive layout.
- **Prominent designer credit** — Eng. Ahmed Yasir shown in the dashboard and banner.
- **Updated AY Zygisk identity** — installer text, WebUI title, metadata and release art.
- **Existing monitoring preserved** — daemon status, Zygote status, module inventory and logs.
- **Integrity validation** — verified installer archive, checksum sidecars and architecture-specific native binaries.
- **Improved usability** — clearer keyboard focus and mobile-size header spacing.

### Downloads / التحميل

- `AY-Zygisk-V3.0.0-BlackGold.zip` — installable root-module ZIP.
- `AY-Zygisk-V3-Banner.png` — Black & Gold artwork.
- `SHA256SUMS.txt` — checksum for the installable ZIP.

### Before installing / قبل التثبيت

1. Keep a current device backup and an accessible recovery method.
2. Do not enable two Zygisk injection engines simultaneously.
3. Install the ZIP using a supported root manager and reboot.
4. Open the WebUI **Status** page to check whether the **runtime and Zygote** are actually active.
5. If inactive, capture root manager and daemon logs before retrying.

> [!WARNING]
> **Pre-release:** Packaging and native binary integrity tests passed in GitHub Actions, but actual Zygote injection has **not** been confirmed on a physical Android device. This edition reuses a pinned, licensed native runtime; visual rebranding is not an independent rewrite. Internal compatibility identifiers remain unchanged. Keep the bundled `LICENSE` and `NOTICE.md` when redistributing.

**Developed visual identity:** Eng. Ahmed Yasir · [AY-Zygisk source](https://github.com/ahmedyasir2023/AY-Zygisk/tree/feat/ay-zygisk-v3-black-gold)
