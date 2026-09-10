#!/bin/sh

# DEPLOYMENT_PREP draft only. Do not copy to the board in this phase.
SERIAL=/dev/s3c2410_serial0

# CQMQUIT is preferred because the monitor restores tty settings and closes fb0.
if ps 2>/dev/null | grep -v grep | grep -q 'codex-monitor'; then
    printf '<CQMQUIT>\n' > "$SERIAL"
    sleep 2
fi

# Recovery is intentionally explicit and uses the validated system entry point.
/bin/qpe.sh &
sleep 2

# today is a Qtopia quicklauncher entry point; this is a fallback only when it
# was not started by qpe.sh. The final wrapper must first check process state.
if ! ps 2>/dev/null | grep -v grep | grep -q '/opt/Qtopia/bin/today'; then
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
