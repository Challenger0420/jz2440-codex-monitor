#!/usr/bin/env bash
set -euo pipefail

ELF=${1:-build/codex-monitor-oabi}
PREFIX=${JZ2440_ARM_PREFIX:-arm-linux-gnueabi-}
READELF=${JZ2440_READELF:-${PREFIX}readelf}
OBJDUMP=${JZ2440_OBJDUMP:-${PREFIX}objdump}

test -f "$ELF" || { echo "ELF not found: $ELF" >&2; exit 1; }
command -v "$READELF" >/dev/null || { echo "readelf not found: $READELF" >&2; exit 127; }
command -v "$OBJDUMP" >/dev/null || { echo "objdump not found: $OBJDUMP" >&2; exit 127; }

echo '=== file ==='
file "$ELF"
echo '=== readelf -h ==='
"$READELF" -h "$ELF"
echo '=== readelf -A ==='
"$READELF" -A "$ELF"
echo '=== readelf -l ==='
"$READELF" -l "$ELF"
echo '=== readelf -d ==='
"$READELF" -d "$ELF"
echo '=== objdump -f ==='
"$OBJDUMP" -f "$ELF"
echo '=== objdump -d ==='
"$OBJDUMP" -d "$ELF" | tee "${ELF}.disasm.txt"
echo '=== ARMv5+ suspicious mnemonic scan ==='
if grep -E '\b(blx|clz|qadd|qsub|qdadd|qdsub|ldrex|strex|cps|bkpt|pld)\b' "${ELF}.disasm.txt"; then
    echo 'suspicious instruction mnemonic found; review the disassembly above.' >&2
    exit 2
else
    echo 'no listed ARMv5+/ARMv6+ mnemonics found'
fi
