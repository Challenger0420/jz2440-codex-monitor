#!/bin/sh

# Manual persistent deployment. This script does not enable autostart.
MONITOR_DIR=/opt/codex-monitor
MONITOR="$MONITOR_DIR/codex-monitor"
SERIAL=/dev/s3c2410_serial0
PIDFILE=/tmp/codex-monitor.pid

monitor_process_present()
{
    ps 2>/dev/null | grep -v grep | grep -q "$MONITOR --serial"
}

qtopia_process_present()
{
    ps 2>/dev/null | grep -v grep | grep -q '/opt/Qtopia/bin/qpe'
}

today_process_present()
{
    ps 2>/dev/null | grep -v grep | grep -q '/opt/Qtopia/bin/today'
}

restore_qtopia()
{
    if ! qtopia_process_present; then
        /bin/qpe.sh &
        sleep 2
    fi

    if ! today_process_present; then
        QTDIR=/opt/Qtopia \
        QPEDIR=/opt/Qtopia \
        QWS_DISPLAY=LinuxFb:/dev/fb0 \
        QWS_KEYBOARD=TTY:/dev/tty1 \
        QWS_MOUSE_PROTO=TPanel:/dev/ts0 \
        TSLIB_TSDEVICE=/dev/ts0 \
        TSLIB_CONSOLEDEVICE=none \
        TSLIB_FBDEVICE=/dev/fb0 \
        TSLIB_CONFFILE=/etc/ts.conf \
        TSLIB_PLUGINDIR=/usr/share/ts/plugins \
        TSLIB_TSEVENTTYPE=H3600 \
        LD_LIBRARY_PATH=/opt/Qtopia/lib:$LD_LIBRARY_PATH \
        /opt/Qtopia/bin/today &
    fi
}

if [ ! -x "$MONITOR" ]; then
    echo "monitor binary missing or not executable: $MONITOR" >&2
    exit 1
fi

if [ -f "$PIDFILE" ]; then
    OLD_PID=`cat "$PIDFILE" 2>/dev/null`
    case "$OLD_PID" in
        ''|*[!0-9]*) rm -f "$PIDFILE" ;;
        *)
            if ps 2>/dev/null | grep -v grep | grep " $OLD_PID " | grep -q "$MONITOR --serial"; then
                echo "monitor already running: $OLD_PID" >&2
                exit 0
            fi
            rm -f "$PIDFILE"
            ;;
    esac
fi

if monitor_process_present; then
    echo "monitor already running without a valid PID file" >&2
    exit 1
fi

# These are the already validated temporary Qtopia stop targets.
killall qpe qss today quicklauncher 2>/dev/null

"$MONITOR" --serial "$SERIAL" &
MONITOR_PID=$!

if ! echo "$MONITOR_PID" > "$PIDFILE"; then
    echo "cannot write PID file: $PIDFILE" >&2
    kill "$MONITOR_PID" 2>/dev/null
    wait "$MONITOR_PID" 2>/dev/null
    restore_qtopia
    exit 1
fi

echo "monitor pid: $MONITOR_PID"

# Keep this supervisor attached to the console so no shell competes for UART input.
wait "$MONITOR_PID"
MONITOR_STATUS=$?

rm -f "$PIDFILE"
restore_qtopia
exit "$MONITOR_STATUS"
