#!/system/bin/sh
# AY Zygisk – independent controller installer
ui_print ' '
ui_print '  AY ZYGISK  //  BLACK & GOLD'
ui_print '  By Eng. Ahmed Yasir'
ui_print '  Independent AY V2 controller and diagnostics'
ui_print ' '
# Some managers do not export API in every install context.
sdk="${API:-}"
[ -n "$sdk" ] || sdk="$(getprop ro.build.version.sdk 2>/dev/null)"
case "$sdk" in
  ''|*[!0-9]*) abort 'Cannot verify Android API level safely.' ;;
esac
if [ "$sdk" -lt 26 ]; then
  abort 'AY Zygisk controller requires Android 8 (API 26) or newer.'
fi
# APatch must be checked first: it may expose compatibility variables.
if [ -n "${APATCH_VER:-}" ] || [ -d /data/adb/apatch ] || [ -d /data/adb/ap ]; then
  ui_print '- Root manager: APatch'
elif [ "${KSU:-}" = true ] || [ "${KSU:-}" = 1 ] || [ -d /data/adb/ksu ]; then
  ui_print '- Root manager: KernelSU / KernelSU Next'
else
  ui_print '- Root manager: Magisk / compatible'
fi
ui_print '- Original Zygisk API bridge source; independent Zygote injector NOT included'
if [ -f "$MODPATH/zygisk/arm64-v8a.so" ] || [ -f "$MODPATH/zygisk/armeabi-v7a.so" ]; then
  ui_print '- Android Zygisk API plugin: included (requires active compatible Zygisk engine)'
else
  ui_print '- Android Zygisk API plugin: NOT packaged (safe controller-only release)'
fi
ui_print '- V2 Doctor, integrity guard and configuration backup included'
ui_print '- Android ABI executable: optional; shell preflight available without NDK'
ui_print '- Existing engines and modules will not be modified'
ui_print '- WebUI: WebUI-compatible manager required'
ui_print '- Diagnostics via action button on supported managers'
mkdir -p "$MODPATH/config" "$MODPATH/reports" "$MODPATH/logs" "$MODPATH/cache"
[ -f "$MODPATH/config/settings.conf" ] || cat > "$MODPATH/config/settings.conf" <<'CONFIG'
AY_MONITOR=1
AY_LOGGING=0
AY_BOOT_SCAN=1
CONFIG
if command -v set_perm >/dev/null 2>&1; then
  set_perm "$MODPATH/action.sh" 0 0 0755
  set_perm "$MODPATH/service.sh" 0 0 0755
  set_perm "$MODPATH/scripts/ayctl.sh" 0 0 0755
fi
for file in "$MODPATH"/bin/*/ay-native; do
  [ -f "$file" ] && chmod 0755 "$file"
done
chmod 0700 "$MODPATH/reports" "$MODPATH/logs" "$MODPATH/cache" "$MODPATH/config" 2>/dev/null || :
ui_print '- AY Zygisk V2 controller installed successfully'
ui_print '- Reboot once before using boot monitoring'
