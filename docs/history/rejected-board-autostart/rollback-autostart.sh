#!/bin/sh

# Restore the exact rcS saved before enabling monitor autostart.
TARGET=/etc/init.d/rcS
BACKUP=/opt/codex-monitor/backup/rcS.original

if [ ! -f "$BACKUP" ]; then
    echo "rollback backup missing: $BACKUP" >&2
    exit 1
fi

cp -p "$BACKUP" "$TARGET" || exit 1
sync
echo "autostart rollback complete: $TARGET restored"
exit 0
