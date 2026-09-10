#!/bin/sh

# DEPLOYMENT_PREP draft only. Do not copy to the board in this phase.
# The final installation directory and binary name are intentionally explicit.
MONITOR_DIR=/home/codex-monitor
MONITOR="$MONITOR_DIR/codex-monitor"
SERIAL=/dev/s3c2410_serial0

# Idempotence guard: a second start must not create another monitor instance.
if ps 2>/dev/null | grep -v grep | grep -q 'codex-monitor'; then
    exit 0
fi

# These are the already validated temporary Qtopia stop targets.
killall qpe qss today quicklauncher 2>/dev/null

exec "$MONITOR" --serial "$SERIAL"
