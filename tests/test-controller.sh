#!/bin/sh
# Desktop smoke tests for the portable shell controller.
set -eu
PROJECT=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT HUP INT TERM
mkdir -p "$WORK/ayzygisk"
cp -R "$PROJECT/scripts" "$PROJECT/config" "$WORK/ayzygisk/"
CTRL="$WORK/ayzygisk/scripts/ayctl.sh"
sh -n "$CTRL"
sh "$CTRL" status | grep -q '^STATUS|native_engine|standalone-not-implemented$'
sh "$CTRL" native | grep -q '^NATIVE|state|shell-fallback-only$'
sh "$CTRL" runtime | grep -q '^RUNTIME|android_zygote_hook|not-implemented$'
sh "$CTRL" runtime | grep -q '^RUNTIME|auto_injection|disabled$'
sh "$CTRL" bridge | grep -q '^BRIDGE|build|not-included-in-zip$'
sh "$CTRL" bridge | grep -q '^BRIDGE|zygote_engine|not-included$'
sh "$CTRL" status | grep -q '^STATUS|manager|'
KSU=1 API=34 sh "$CTRL" ksun | grep -q '^KSUN|android|pass-api-34$'
KSU=1 API=33 sh "$CTRL" ksun | grep -q '^KSUN|android|unsupported-api-33$'
KSU=1 API=34 sh "$CTRL" ksun | grep -q '^KSUN|root_family|kernelsu-detected-next-unverified$'
KSU=1 KSU_LATE_LOAD=1 API=34 sh "$CTRL" ksun | grep -q '^KSUN|boot_mode|late-load-not-eligible-for-early-zygote$'
KSU=1 API=34 sh "$CTRL" ksun | grep -q '^KSUN|injection|disabled$'
sh "$CTRL" config | grep -q '^CONFIG|AY_MONITOR|1$'
sh "$CTRL" set AY_LOGGING 1 | grep -q '^OK|AY_LOGGING|1$'
sh "$CTRL" config | grep -q '^CONFIG|AY_LOGGING|1$'
if sh "$CTRL" set "AY_LOGGING;rm -rf /" 1 >/dev/null 2>&1; then echo 'Rejected key accepted' >&2; exit 1; fi
if sh "$CTRL" set AY_LOGGING 2 >/dev/null 2>&1; then echo 'Rejected value accepted' >&2; exit 1; fi
sh "$CTRL" boot-scan
[ -f "$WORK/ayzygisk/cache/inventory-fingerprint" ]
sh "$CTRL" report | grep -q '^AY ZYGISK | DIAGNOSTIC REPORT$'
[ -s "$WORK/ayzygisk/reports/health-latest.txt" ]
sh "$CTRL" logs | grep -q 'Boot scan completed'
sh "$CTRL" scan | grep -q '^SUMMARY|engine_count|'
sh "$CTRL" doctor | grep -q '^CHECK|standalone_engine|not-implemented|'
sh "$CTRL" backup-settings | grep -q '^BACKUP|saved|'
sh "$CTRL" set AY_LOGGING 0 | grep -q '^OK|AY_LOGGING|0$'
sh "$CTRL" restore-settings | grep -q '^BACKUP|restored|'
sh "$CTRL" config | grep -q '^CONFIG|AY_LOGGING|1$'

echo 'PASS: portable controller + Runtime readiness and native fallback tests'
