# Rejected Board Autostart experiment

The earlier `rcS → boot-monitor.sh → Monitor` experiment was rolled back.
The legacy BusyBox `askfirst` shell and the foreground Monitor both consumed
the same UART RX path, so safe Console/Application ownership and host-driven
exit could not be guaranteed.

The original helper files are retained here for historical reference only.
They are not part of the supported platform deployment path, and Board
Autostart remains disabled.
