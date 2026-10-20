#!/system/bin/sh
MODDIR=${0%/*}
# Non-invasive, one-shot startup scan. No Zygote hooks or persistent daemon.
[ -f "$MODDIR/scripts/ayctl.sh" ] || exit 0
/system/bin/sh "$MODDIR/scripts/ayctl.sh" boot-scan >/dev/null 2>&1
exit 0
