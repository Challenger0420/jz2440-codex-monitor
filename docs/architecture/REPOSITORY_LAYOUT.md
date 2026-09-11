# Repository layout

```text
apps/
  codex-monitor/       Current application: display, protocol, host Bridge
platform/
  board/appctl/         Generic BusyBox foreground launcher
  host/board-controller Serial transport, discovery, mode state machine
  target/runtime/       ARMv4T/OABI startup and syscall runtime
  target/diagnostics/   Small target diagnostics and probes
  target/renderer/      Reserved for a future independently reusable renderer
  target/font/          Reserved for a future independently reusable font engine
scripts/
  build/                Target/Bridge build and ELF inspection entry points
  deploy/               Explicit future deployment helpers
  verify/               Explicit future field verification helpers
tools/
  preview_host_include/ Host-only framebuffer compatibility headers
docs/
  architecture/         Platform and repository design
  deployment/           Build and historical deployment references
  history/              Rejected experiments and migration notes
third_party/
  fonts/oxanium/        Upstream font assets and SIL OFL license
```

Generated binaries, previews, logs, and intermediate objects remain under
`build/` and are not repository source. Only the Codex-specific renderer and
generated glyph data currently stay in the App because no second consumer has
yet justified extraction into a shared API. Future applications may add their
own app-specific directories without changing the shared platform boundary.
