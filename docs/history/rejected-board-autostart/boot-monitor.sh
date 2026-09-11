#!/bin/sh

# Minimal boot hook. It is invoked from rcS and falls back to the original
# Qtopia launcher if the monitor cannot be started.
LOG=/tmp/codex-monitor-boot.log
MONITOR_DIR=/opt/codex-monitor
START="$MONITOR_DIR/start-monitor.sh"

if [ -f "$LOG" ]; then
    mv "$LOG" "$LOG.1" 2>/dev/null
fi

fallback_qtopia()
{
    if ! ps 2>/dev/null | grep -v grep | grep -q '/opt/Qtopia/bin/qpe'; then
        /bin/qpe.sh &
    fi
}

if [ ! -x "$MONITOR_DIR/codex-monitor" ] || [ ! -x "$START" ]; then
    echo "monitor or start script unavailable; falling back to Qtopia" >> "$LOG"
    fallback_qtopia
    exit 1
fi

echo "starting monitor" >> "$LOG"
"$START" >> "$LOG" 2>&1
STATUS=$?

if [ "$STATUS" -ne 0 ]; then
    echo "monitor start failed ($STATUS); falling back to Qtopia" >> "$LOG"
    fallback_qtopia
fi

exit "$STATUS"
