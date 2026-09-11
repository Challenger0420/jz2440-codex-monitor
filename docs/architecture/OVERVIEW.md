# Architecture overview

```text
Windows
  ↓
Board Controller
  ↓
PL2303 serial transport
  ↓
JZ2440 serial console
  ↓
Generic appctl
  ↓
Foreground Application Mode
```

The board keeps its original Qtopia and serial console. The single UART is
time-multiplexed:

```text
Console Mode  ↔  Application Mode
```

In Console Mode, the host may perform a bounded console synchronization and
issue a controlled `appctl` command. `appctl` stops Qtopia, emits an
application-ready marker, and waits synchronously for the foreground app. The
shell therefore does not become a second UART reader while the application
owns RX. On application exit, cleanup restores Qtopia and emits an app-stop
marker.

The generic Board Controller refuses to act in `UNKNOWN` mode. A registered
application adapter supplies its lifecycle markers and application protocol;
the first adapter is Codex Monitor. Its quota path remains request-driven:
`CQMREQ` from the board causes one real `CQM1` response from the host.

Board Autostart through the legacy `askfirst`/`rcS` path is explicitly
rejected because it creates competing UART readers. Autostart is not part of
the current platform baseline.
