#!/system/bin/sh
MODDIR=${0%/*}
printf '===================================================\n'
printf '  AY ZYGISK | BLACK & GOLD | ENG. AHMED YASIR\n'
printf '===================================================\n'
printf 'AY V2 health and diagnostics – no Zygote injection\n\n'
/system/bin/sh "$MODDIR/scripts/ayctl.sh" report
