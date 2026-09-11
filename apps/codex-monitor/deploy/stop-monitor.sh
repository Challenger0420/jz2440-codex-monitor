#!/bin/sh

# Local PID fallback for an independent shell/control channel.
# The host-side canonical stop path is a CQMQUIT sent into the UART RX.
MONITOR=/opt/codex-monitor/codex-monitor
PIDFILE=/tmp/codex-monitor.pid

pid_matches_monitor()
{
    PID="$1"
    case "$PID" in
        ''|*[!0-9]*) return 1 ;;
    esac
    ps 2>/dev/null | grep -v grep | grep " $PID " | grep -q "$MONITOR --serial"
}

qtopia_process_present()
{
    ps 2>/dev/null | grep -v grep | grep -q '/opt/Qtopia/bin/qpe'
}

today_process_present()
{
    ps 2>/dev/null | grep -v grep | grep -q '/opt/Qtopia/bin/today'
}

recover_qtopia_if_needed()
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

if [ ! -f "$PIDFILE" ]; then
    recover_qtopia_if_needed
    exit 0
fi

PID=`cat "$PIDFILE" 2>/dev/null`
if ! pid_matches_monitor "$PID"; then
    rm -f "$PIDFILE"
    recover_qtopia_if_needed
    exit 0
fi

kill "$PID" 2>/dev/null || exit 1

i=0
while pid_matches_monitor "$PID"; do
    sleep 1
    i=`expr "$i" + 1`
    if [ "$i" -ge 5 ]; then
        kill -9 "$PID" 2>/dev/null
        sleep 1
        if pid_matches_monitor "$PID"; then
            echo "monitor did not exit after TERM/KILL: $PID" >&2
            exit 1
        fi
        break
    fi
done

rm -f "$PIDFILE"

# Normally the start supervisor performs recovery after wait returns.
# Recover only if that supervisor is absent or did not restore Qtopia.
if ! qtopia_process_present; then
    recover_qtopia_if_needed
fi
exit 0
