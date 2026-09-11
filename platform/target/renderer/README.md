# Shared renderer boundary

This directory is reserved for framebuffer primitives that have a second real
application consumer. The current Codex Monitor renderer remains under
`apps/codex-monitor/target/src/codex_monitor.c` to preserve its validated
behavior and avoid speculative abstraction.
