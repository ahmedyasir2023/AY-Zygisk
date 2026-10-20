#!/system/bin/sh
# AY Zygisk V2 diagnostics, Native Lab, Doctor, integrity guard and API bridge readiness.
set -u
HERE=${0%/*}
MODDIR=${HERE%/scripts}
CONFIG="$MODDIR/config/settings.conf"
MANIFEST="$MODDIR/integrity.sha256"
SETTINGS_BACKUP="$MODDIR/reports/settings-backup.conf"
REPORT_DIR="$MODDIR/reports"
LOG_DIR="$MODDIR/logs"
mkdir -p "$REPORT_DIR" "$LOG_DIR" "$MODDIR/config" 2>/dev/null || :
[ -f "$CONFIG" ] || printf 'AY_MONITOR=1\nAY_LOGGING=0\nAY_BOOT_SCAN=1\n' > "$CONFIG"

sanitize() { printf '%s' "$1" | tr '\r\n|' '   ' | cut -c 1-140; }
propval() { getprop "$1" 2>/dev/null || printf ''; }
setting() {
  key="$1"
  val=$(sed -n "s/^${key}=//p" "$CONFIG" 2>/dev/null | head -n 1)
  case "$val" in 0|1) printf '%s' "$val" ;; *) printf '0' ;; esac
}
manager() {
  # APatch may ship Magisk compatibility variables: test it first.
  if [ -d /data/adb/ap ] || [ -d /data/adb/apatch ]; then printf 'APatch'
  elif [ -d /data/adb/ksu ] || command -v ksud >/dev/null 2>&1; then printf 'KernelSU'
  elif command -v magisk >/dev/null 2>&1 || [ -d /data/adb/magisk ]; then printf 'Magisk'
  else printf 'Undetected'; fi
}
roots() {
  for root in /data/adb/modules /data/adb/ksu/modules /data/adb/apatch/modules; do
    [ -d "$root" ] && printf '%s\n' "$root"
  done
}
installed_modules() {
  roots | while IFS= read -r root; do
    for dir in "$root"/*; do
      [ -d "$dir" ] && [ -f "$dir/module.prop" ] || continue
      id=${dir##*/}
      [ "$id" = 'ayzygisk' ] && continue
      name=$(sed -n 's/^name=//p' "$dir/module.prop" | head -n 1)
      [ -z "$name" ] && name="$id"
      status=enabled
      [ -f "$dir/disable" ] && status=disabled
      [ -f "$dir/remove" ] && status=removing
      api=no
      [ -d "$dir/zygisk" ] && api=yes
      printf 'MODULE|%s|%s|%s|%s\n' "$(sanitize "$id")" "$(sanitize "$name")" "$status" "$api"
    done
  done
}
engines() {
  # Installed markers, not a claim about active Zygote injection.
  for id in zygisksu rezygisk zygisk_next zygisknext; do
    for root in /data/adb/modules /data/adb/ksu/modules /data/adb/apatch/modules; do
      if [ -f "$root/$id/module.prop" ] && [ ! -e "$root/$id/disable" ]; then
        printf '%s' "$id"
        return
      fi
    done
  done
  printf 'not-detected'
}
status() {
  printf 'STATUS|manager|%s\n' "$(sanitize "$(manager)")"
  printf 'STATUS|android|%s\n' "$(sanitize "$(propval ro.build.version.release)")"
  printf 'STATUS|sdk|%s\n' "$(sanitize "$(propval ro.build.version.sdk)")"
  printf 'STATUS|model|%s\n' "$(sanitize "$(propval ro.product.model)")"
  printf 'STATUS|architecture|%s\n' "$(sanitize "$(propval ro.product.cpu.abi)")"
  printf 'STATUS|backend|%s\n' "$(engines)"
  uptime_seconds=$(cut -d. -f1 /proc/uptime 2>/dev/null | cut -d' ' -f1)
  printf 'STATUS|uptime|%s\n' "${uptime_seconds:-0}"
  free_kb=$(awk '/MemAvailable:/ {print $2; exit}' /proc/meminfo 2>/dev/null)
  printf 'STATUS|memory_mb|%s\n' "$((${free_kb:-0} / 1024))"
  printf 'STATUS|monitor|%s\n' "$(setting AY_MONITOR)"
  printf 'STATUS|logging|%s\n' "$(setting AY_LOGGING)"
  printf 'STATUS|bootscan|%s\n' "$(setting AY_BOOT_SCAN)"
  printf 'STATUS|release|2.0.0\n'
  printf 'STATUS|native_engine|standalone-not-implemented\n'
  if [ -f "$MODDIR/zygisk/$(device_abi).so" ]; then
    printf 'STATUS|api_bridge|android-plugin-packaged-unverified\n'
  else
    printf 'STATUS|api_bridge|not-packaged\n'
  fi
  printf 'STATUS|runtime_lab|host-test-source-included\n'
  if [ -x "$MODDIR/bin/$(device_abi)/ay-native" ]; then
    printf 'STATUS|native_lab|android-binary-present\n'
  else
    printf 'STATUS|native_lab|shell-fallback-only\n'
  fi
  changed=0
  [ -f "$MODDIR/cache/inventory-changed" ] && changed=1
  printf 'STATUS|inventory_changed|%s\n' "$changed"
}
scan() {
  installed_modules
  # A warning is raised only for observable duplicate *installed* engines.
  count=0
  for id in zygisksu rezygisk zygisk_next zygisknext; do
    for root in /data/adb/modules /data/adb/ksu/modules /data/adb/apatch/modules; do
      if [ -f "$root/$id/module.prop" ] && [ ! -e "$root/$id/disable" ]; then count=$((count+1)); fi
    done
  done
  if [ "$count" -gt 1 ]; then
    printf 'WARNING|Multiple standalone Zygisk engines appear installed and enabled.\n'
  fi
  printf 'SUMMARY|engine_count|%s\n' "$count"
}
# Native Lab selects an executable only when it matches the device's ABI. Android
# binaries are never substituted with the Linux host test binary.
device_abi() {
  abi=$(propval ro.product.cpu.abi)
  case "$abi" in
    arm64-v8a|armeabi-v7a|x86|x86_64|riscv64) printf '%s' "$abi" ;;
    *) printf 'unknown' ;;
  esac
}
# Shell preflight is deliberately limited to ELF header checks; it never claims
# that a library exports the full Zygisk API or is safe to load.
basic_elf_check() {
  file="$1"; expected="$2"
  if ! command -v od >/dev/null 2>&1; then printf 'preflight-unavailable'; return; fi
  bytes=$(od -An -tu1 -N20 "$file" 2>/dev/null) || { printf 'unreadable'; return; }
  set -- $bytes
  [ "$#" -ge 20 ] || { printf 'invalid-elf'; return; }
  [ "$1" = 127 ] && [ "$2" = 69 ] && [ "$3" = 76 ] && [ "$4" = 70 ] || { printf 'invalid-elf'; return; }
  [ "$6" = 1 ] && [ "${17}" = 3 ] && [ "${18}" = 0 ] && [ "${20}" = 0 ] || { printf 'invalid-elf'; return; }
  case "$expected" in
    arm64-v8a) [ "$5" = 2 ] && [ "${19}" = 183 ] ;;
    armeabi-v7a) [ "$5" = 1 ] && [ "${19}" = 40 ] ;;
    x86) [ "$5" = 1 ] && [ "${19}" = 3 ] ;;
    x86_64) [ "$5" = 2 ] && [ "${19}" = 62 ] ;;
    riscv64) [ "$5" = 2 ] && [ "${19}" = 243 ] ;;
    *) return 1 ;;
  esac && { printf 'elf-header-match'; return; }
  printf 'architecture-mismatch'
}
native_scan() {
  abi=$(device_abi)
  native_bin="$MODDIR/bin/$abi/ay-native"
  if [ -x "$native_bin" ]; then
    printf 'NATIVE|state|android-binary-present\n'
    printf 'NATIVE|mode|read-only-preflight\n'
    roots | while IFS= read -r root; do
      "$native_bin" inventory "$root" 2>/dev/null || :
    done
  else
    printf 'NATIVE|state|shell-fallback-only\n'
    printf 'NATIVE|mode|basic-elf-header-check\n'
    printf 'NATIVE|message|Full native export verification requires Android NDK build\n'
    count=0
    roots | while IFS= read -r root; do
      for dir in "$root"/*; do
        [ -d "$dir" ] && [ -f "$dir/module.prop" ] || continue
        [ -d "$dir/zygisk" ] || continue
        id=${dir##*/}
        for known in arm64-v8a armeabi-v7a x86 x86_64 riscv64; do
          so="$dir/zygisk/$known.so"
          [ -f "$so" ] || continue
          result=$(basic_elf_check "$so" "$known")
          printf 'LIB|%s|%s|%s|not-verified|not-verified|0\n' "$(sanitize "$id")" "$known" "$result"
        done
      done
    done
  fi
}
runtime_status() {
  printf 'RUNTIME|release|2.0.0\n'
  printf 'RUNTIME|device_mode|read-only-readiness\n'
  printf 'RUNTIME|host_lab|isolated-AY-fixtures-source\n'
  printf 'RUNTIME|android_zygote_hook|not-implemented\n'
  printf 'RUNTIME|zygisk_api_callbacks|source-implemented-api-v5-android-unverified\n'
  printf 'RUNTIME|auto_injection|disabled\n'
  printf 'RUNTIME|root_manager|%s\n' "$(sanitize "$(manager)")"
  printf 'RUNTIME|cpu_abi|%s\n' "$(sanitize "$(device_abi)")"
  if [ -x "$MODDIR/bin/$(device_abi)/ay-native" ]; then
    printf 'RUNTIME|android_inspector|present\n'
  else
    printf 'RUNTIME|android_inspector|not-built\n'
  fi
  printf 'RUNTIME|backend_marker|%s\n' "$(engines)"
  printf 'RUNTIME|next_milestone|android-native-bridge-and-device-tests\n'
}
bridge_status() {
  abi=$(device_abi)
  bridge="$MODDIR/zygisk/$abi.so"
  printf 'BRIDGE|release|2.0.0\n'
  printf 'BRIDGE|module_id|ayzygisk\n'
  printf 'BRIDGE|zygisk_public_api|v5\n'
  printf 'BRIDGE|backend_marker|%s\n' "$(engines)"
  printf 'BRIDGE|cpu_abi|%s\n' "$abi"
  if [ -f "$bridge" ]; then
    printf 'BRIDGE|build|android-library-present\n'
    printf 'BRIDGE|file_size|%s\n' "$(wc -c < "$bridge" | tr -d ' ')"
  else
    printf 'BRIDGE|build|not-included-in-zip\n'
  fi
  printf 'BRIDGE|zygote_engine|not-included\n'
  printf 'BRIDGE|live_callbacks|not-device-verified\n'
  printf 'BRIDGE|features|non-intrusive-smoke-test-only\n'
  printf 'BRIDGE|security_bypass|none\n'
}
config() { for key in AY_MONITOR AY_LOGGING AY_BOOT_SCAN; do printf 'CONFIG|%s|%s\n' "$key" "$(setting "$key")"; done; }
set_config() {
  key=${1:-}; val=${2:-}
  case "$key" in AY_MONITOR|AY_LOGGING|AY_BOOT_SCAN) : ;; *) printf 'ERROR|Invalid setting\n'; return 2 ;; esac
  case "$val" in 0|1) : ;; *) printf 'ERROR|Invalid value\n'; return 2 ;; esac
  # Atomic change, fixed keys and values, no arbitrary shell execution.
  tmp="$CONFIG.tmp.$$"
  grep -v "^${key}=" "$CONFIG" > "$tmp" 2>/dev/null || :
  printf '%s=%s\n' "$key" "$val" >> "$tmp"
  if mv -f "$tmp" "$CONFIG"; then printf 'OK|%s|%s\n' "$key" "$val"; else printf 'ERROR|Save failed\n'; return 1; fi
}
event() {
  [ "$(setting AY_LOGGING)" = '1' ] || return 0
  [ ! -f "$LOG_DIR/events.log" ] || [ "$(wc -c < "$LOG_DIR/events.log")" -lt 131072 ] || mv "$LOG_DIR/events.log" "$LOG_DIR/events.old.log" 2>/dev/null || :
  printf '%s | %s\n' "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null)" "$1" >> "$LOG_DIR/events.log"
}
report() {
  file="$REPORT_DIR/health-latest.txt"
  {
    printf 'AY ZYGISK | DIAGNOSTIC REPORT\n'
    printf 'Eng. Ahmed Yasir | AY V2 independent controller\n'
    printf 'Generated: %s\n' "$(date '+%Y-%m-%d %H:%M:%S')"
    printf 'Note: Does not contain installed app lists or private data.\n'
    printf 'Note: Backend markers do not prove active Zygote injection.\n\n'
    status
    printf '\nMODULE SCAN\n'
    scan
    printf '\nNATIVE LAB\n'
    native_scan
    printf '\nRUNTIME READINESS\n'
    runtime_status
    printf '\nPUBLIC ZYGISK API BRIDGE\n'
    bridge_status
    printf '\nAY V2 DOCTOR\n'
    doctor
    printf '\nINTEGRITY GUARD\n'
    verify
  } > "$file"
  event 'Generated a diagnostic report'
  printf 'REPORT|%s\n' "$file"
  cat "$file"
}
boot_scan() {
  [ "$(setting AY_MONITOR)" = '1' ] || return 0
  [ "$(setting AY_BOOT_SCAN)" = '1' ] || return 0
  mkdir -p "$MODDIR/cache" 2>/dev/null || :
  current=$(installed_modules | LC_ALL=C sort | cksum | awk '{print $1}')
  previous=$(cat "$MODDIR/cache/inventory-fingerprint" 2>/dev/null || :)
  if [ -n "$previous" ] && [ "$previous" != "$current" ]; then
    : > "$MODDIR/cache/inventory-changed"
    event 'Module inventory changed since last startup'
  else
    rm -f "$MODDIR/cache/inventory-changed"
  fi
  printf '%s\n' "$current" > "$MODDIR/cache/inventory-fingerprint"
  report >/dev/null
  event 'Boot scan completed'
}

# SHA-256 integrity manifest covers the project files, not installed third-party modules.
# Values are created at release build time; absence of hashing support is reported.
hash_file() {
  file=$1
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$file" 2>/dev/null | awk '{print $1}'
  elif command -v toybox >/dev/null 2>&1; then
    toybox sha256sum "$file" 2>/dev/null | awk '{print $1}'
  elif command -v busybox >/dev/null 2>&1; then
    busybox sha256sum "$file" 2>/dev/null | awk '{print $1}'
  else return 127; fi
}
verify() {
  if [ ! -f "$MANIFEST" ]; then
    printf 'VERIFY|state|manifest-missing\n'; return 1
  fi
  if ! command -v sha256sum >/dev/null 2>&1 && ! command -v toybox >/dev/null 2>&1 && ! command -v busybox >/dev/null 2>&1; then
    printf 'VERIFY|state|hash-tool-unavailable\n'; return 2
  fi
  pass=0; fail=0
  while IFS=' ' read -r expected relative || [ -n "$expected" ]; do
    [ -n "${expected:-}" ] || continue
    # Allow only fixed release paths; reject traversal and special characters.
    case "$relative" in
      module.prop|customize.sh|action.sh|service.sh|uninstall.sh|scripts/ayctl.sh|webroot/index.html|webroot/assets/app.css|webroot/assets/app.js|webroot/assets/mark.svg) : ;;
      *) printf 'VERIFY|unsafe-entry|%s\n' "$(sanitize "$relative")"; fail=$((fail+1)); continue ;;
    esac
    case "$expected" in *[!0-9a-f]*|'') fail=$((fail+1)); continue ;; esac
    actual=$(hash_file "$MODDIR/$relative")
    if [ -n "$actual" ] && [ "$expected" = "$actual" ]; then pass=$((pass+1))
    else
      printf 'VERIFY|mismatch|%s\n' "$relative"
      fail=$((fail+1))
    fi
  done < "$MANIFEST"
  printf 'VERIFY|verified|%s\nVERIFY|failed|%s\n' "$pass" "$fail"
  [ "$fail" -eq 0 ] && [ "$pass" -eq 10 ] && { printf 'VERIFY|state|passed\n'; return 0; }
  printf 'VERIFY|state|failed\n'; return 1
}
# Read-only compatibility doctor. Backend installation markers cannot prove
# active Zygisk runtime or successful callbacks in device processes.
doctor() {
  printf 'DOCTOR|version|2.0.0\n'
  printf 'DOCTOR|module_id|ayzygisk\n'
  printf 'DOCTOR|root_manager|%s\n' "$(sanitize "$(manager)")"
  sdk=$(propval ro.build.version.sdk)
  case "$sdk" in
    ''|*[!0-9]*) printf 'CHECK|android|unknown|Android SDK unavailable\n' ;;
    *) if [ "$sdk" -ge 26 ]; then printf 'CHECK|android|pass|Android API %s\n' "$sdk"
       else printf 'CHECK|android|fail|Android API below 26\n'; fi ;;
  esac
  mgr=$(manager)
  case "$mgr" in
    APatch|KernelSU|Magisk) printf 'CHECK|manager|pass|%s module manager marker\n' "$mgr" ;;
    *) printf 'CHECK|manager|unknown|Root manager not detectable\n' ;;
  esac
  abi=$(device_abi)
  case "$abi" in unknown) printf 'CHECK|abi|unknown|Device ABI unavailable\n' ;; *) printf 'CHECK|abi|pass|%s\n' "$abi" ;; esac
  engine=$(engines)
  if [ "$engine" = 'not-detected' ]; then
    printf 'CHECK|zygisk_backend|unknown|No independent backend marker found\n'
  else
    printf 'CHECK|zygisk_backend|warn|Installed marker %s; runtime not verified\n' "$engine"
  fi
  if [ -f "$MODDIR/zygisk/$abi.so" ]; then
    printf 'CHECK|ay_api_bridge|warn|Library packaged; callbacks not device-verified\n'
  else
    printf 'CHECK|ay_api_bridge|not-included|Zygisk library is not shipped\n'
  fi
  printf 'CHECK|standalone_engine|not-implemented|No standalone Zygote injector included\n'
  if [ -d "$REPORT_DIR" ] && [ -w "$REPORT_DIR" ]; then
    printf 'CHECK|diagnostics|pass|Local report directory writable\n'
  else
    printf 'CHECK|diagnostics|fail|Local report directory not writable\n'
  fi
  printf 'DOCTOR|security_bypass|none\n'
}
backup_settings() {
  mkdir -p "$REPORT_DIR" || return 1
  tmp="$SETTINGS_BACKUP.tmp.$$"
  config > "$tmp" || return 1
  # Export only known binary settings, never arbitrary lines from config.
  sed -n 's/^CONFIG|\(AY_[A-Z_]*\)|\([01]\)$/\1=\2/p' "$tmp" > "$tmp.clean"
  mv -f "$tmp.clean" "$SETTINGS_BACKUP" || return 1
  rm -f "$tmp"
  chmod 0600 "$SETTINGS_BACKUP" 2>/dev/null || :
  printf 'BACKUP|saved|%s\n' "$SETTINGS_BACKUP"
}
restore_settings() {
  [ -f "$SETTINGS_BACKUP" ] || { printf 'ERROR|No AY settings backup found\n'; return 1; }
  for k in AY_MONITOR AY_LOGGING AY_BOOT_SCAN; do
    v=$(sed -n "s/^${k}=//p" "$SETTINGS_BACKUP" | head -n 1)
    case "$v" in 0|1) : ;; *) printf 'ERROR|Invalid backup content\n'; return 1 ;; esac
  done
  for k in AY_MONITOR AY_LOGGING AY_BOOT_SCAN; do
    v=$(sed -n "s/^${k}=//p" "$SETTINGS_BACKUP" | head -n 1)
    set_config "$k" "$v" >/dev/null || return 1
  done
  printf 'BACKUP|restored|AY settings only\n'
}

case "${1:-}" in
  status) status ;;
  scan) scan ;;
  config) config ;;
  set) set_config "${2:-}" "${3:-}" ;;
  report) report ;;
  native) native_scan ;;
  runtime) runtime_status ;;
  bridge) bridge_status ;;
  boot-scan) boot_scan ;;
  doctor) doctor ;;
  verify) verify ;;
  backup-settings) backup_settings ;;
  restore-settings) restore_settings ;;
  logs) [ -f "$LOG_DIR/events.log" ] && tail -n 60 "$LOG_DIR/events.log" || printf 'INFO|No events recorded\n' ;;
  *) printf 'Usage: ayctl.sh {status|scan|native|runtime|bridge|doctor|verify|config|set KEY 0/1|backup-settings|restore-settings|report|logs|boot-scan}\n'; exit 2 ;;
esac
