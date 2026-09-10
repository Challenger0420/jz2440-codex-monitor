#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
CC=${JZ2440_ARM_GCC:-arm-linux-gnueabi-gcc}
BUILD_DIR=${JZ2440_BUILD_DIR:-build}
CONFIG="$ROOT/scripts/target-flags.txt"

if ! command -v "$CC" >/dev/null 2>&1 && [ ! -x "$CC" ]; then
    echo "ARM GCC not found: $CC" >&2
    echo "Set JZ2440_ARM_GCC to the original ARMv4T/OABI-capable gcc." >&2
    exit 127
fi

cflags=()
ldflags=()
section=""
while IFS= read -r line || [ -n "$line" ]; do
    case "$line" in
        '[CFLAGS]') section=cflags ;;
        '[LDFLAGS]') section=ldflags ;;
        ''|'#'*) ;;
        *)
            if [ "$section" = cflags ]; then cflags+=("$line"); fi
            if [ "$section" = ldflags ]; then ldflags+=("$line"); fi
            ;;
    esac
done < "$CONFIG"

mkdir -p "$BUILD_DIR"
rm -f "$BUILD_DIR/oabi_start.o" "$BUILD_DIR/codex-monitor-oabi"
"$CC" "${cflags[@]}" -c src/oabi_start.S -o "$BUILD_DIR/oabi_start.o"
"$CC" "${cflags[@]}" "${ldflags[@]}" -o "$BUILD_DIR/codex-monitor-oabi" \
    "$BUILD_DIR/oabi_start.o" src/oabi_runtime.c src/codex_monitor.c -lgcc
echo "built $BUILD_DIR/codex-monitor-oabi"
