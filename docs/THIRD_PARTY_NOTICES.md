# Third-party notices

## Oxanium

This project uses the Oxanium typeface by its original authors, distributed
under the SIL Open Font License 1.1 (OFL-1.1).

- Source font files: `third_party/fonts/oxanium/Oxanium-Bold.ttf` and `third_party/fonts/oxanium/Oxanium-SemiBold.ttf`
- License text: `third_party/fonts/oxanium/OFL.txt`
- Usage: the fonts are rasterized at build time into bitmap glyph data in `apps/codex-monitor/target/include/font_data.h`.
- The JZ2440 target contains generated bitmap glyph data; it does not parse TTF files at runtime.

Oxanium is not project-owned code and is not relicensed by this repository.
