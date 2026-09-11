# JZ2440 App Platform

JZ2440 App Platform is a lightweight application platform for the
JZ2440/S3C2440A. It provides a reusable board launcher, Windows-side board
controller, serial application mode, framebuffer runtime, and target build
tooling for multiple embedded applications.

Current applications:

- Codex Monitor: the first validated application.
- Future applications, including Research Status, are not implemented here.

The GitHub remote remains `Challenger0420/jz2440-codex-monitor` for now. The
repository has not been renamed, committed, or pushed by this restructuring.

## Platform layout

- `platform/board/appctl/`: BusyBox-compatible foreground application launcher.
- `platform/host/board-controller/`: serial transport, PL2303 discovery, and Console/Application mode controller.
- `platform/target/runtime/`: ARMv4T/ARM920T freestanding OABI startup and syscall runtime.
- `platform/target/renderer/`: reserved for renderer extraction after a second real consumer exists; the current renderer stays app-local to preserve behavior.
- `apps/codex-monitor/`: Codex-specific target, host, protocol, and UI.
- `third_party/fonts/oxanium/`: Oxanium font sources and SIL OFL license.
- `scripts/build/`, `scripts/deploy/`, `scripts/verify/`: reproducible build and future field workflows.

The single UART is time-multiplexed: Console Mode hands control to a
foreground application, the shell waits while the application owns RX, and
the application exit path restores Qtopia. Board autostart through `rcS` is a
rejected design because the legacy `askfirst` shell competes for the same UART.

## Codex Monitor

See [apps/codex-monitor/README.md](apps/codex-monitor/README.md) for the
validated display, CQM request/response protocol, host Bridge, and current
hardware/software gates.

## Build and preview

Target compilation uses the validated Ubuntu ARM GNU toolchain and frozen
flags in [scripts/build/target-flags.txt](scripts/build/target-flags.txt):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build\build-target.ps1
powershell -ExecutionPolicy Bypass -File scripts\build\build-bridge.ps1
python apps\codex-monitor\tools\preview.py
```

The root Makefile also provides `make codex-monitor`, `make preview`,
`make status-preview`, and `make bridge` where the corresponding toolchain is
available. Generated outputs remain under `build/` and are not committed.

## Current handoff status

- `APP_PLATFORM_DESIGN_GATE = PASS`
- `APPCTL_IMPLEMENTATION_GATE = PASS`
- `APPCTL_STATIC_COMPAT_GATE = PASS`
- `BOARD_CONTROLLER_STATE_MACHINE_GATE = PASS`
- `BOARD_CONTROLLER_SIMULATION_GATE = PASS`
- `CODEX_ADAPTER_GATE = PASS`
- `REQUEST_RESPONSE_PROTOCOL_GATE = PASS`
- `HOST_TX_SILENCE_GATE = PASS`
- `STATUS_UI_PREVIEW_GATE = PASS`
- `STATUS_BUILD_GATE = PASS`
- `DEPLOYMENT_SCRIPT_PREP_GATE = PASS`
- `USB_RECONNECT_FINAL_GATE = PASS`
- `STATUS_HARDWARE_GATE = PASS`
- `WINDOWS_AUTOSTART_GATE = DEFERRED`

The recorded final Codex Monitor target is 51400 bytes with SHA256
`8C2C20DB7E2C1217B385E4BADADDF1B9769C9C7C2EC96ED6FA26290EBE92EA35`.
The board safe baseline remains Qtopia running and Monitor stopped. No
U-Boot, bootargs, kernel, NAND, rootfs, init, Qtopia startup file, or remote
repository setting was changed by this restructuring.

More detail is in [docs/CURRENT_STATUS.md](docs/CURRENT_STATUS.md),
[docs/architecture/OVERVIEW.md](docs/architecture/OVERVIEW.md), and
[docs/architecture/REPOSITORY_LAYOUT.md](docs/architecture/REPOSITORY_LAYOUT.md).
