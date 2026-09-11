# Font refinement

字体只在主机端构建时栅格化，目标板不解析 TTF，也不依赖 FreeType、Qt、SDL 或系统字体。当前采用固定提交 `a8f39e0c71186190027a093e9001459410192d1e` 的 Oxanium：Header/Large Number 使用 Bold，Label/Reset Time 使用 SemiBold。字体来源为上游 `sevmeyer/oxanium`，随附 `third_party/fonts/oxanium/OFL.txt`，授权为 SIL Open Font License 1.1。

## 字体表

`apps/codex-monitor/tools/generate_font.py` 使用 Pillow 生成 `apps/codex-monitor/target/include/font_data.h`。普通角色字符集覆盖空格、`0123456789%/:.-` 和 `A-Z`；Large Number 角色单独使用 `0123456789%-`，其中 `%` 是 ASCII `0x25` 的独立大号 glyph，不会 fallback 到 `/`。Header/Label/Reset Time 保存 8-bit alpha bitmap，Large Number 保留 1-bit bitmap；所有角色都保存宽度、高度和 advance，运行时由 `measure_text()` 按 advance 计算宽度并完成居中/右对齐。

当前四类角色：

| 角色 | 字体 | 主机栅格字号 | 位图数据 |
| --- | --- | ---: | ---: |
| Header | Oxanium Bold | 28 px | 13838 bytes alpha |
| Label | Oxanium SemiBold | 15 px | 4119 bytes alpha |
| Large Number | Oxanium Bold | 42 px | 1047 bytes |
| Reset Time | Oxanium SemiBold | 27 px | 12700 bytes alpha |

合计字体位图数据为 31704 bytes；目标端仅使用生成后的 C 表。Alpha glyph 使用 8-bit `uint8_t alpha[]`，RGB565 混合在目标端使用整数计算，无浮点。

## 可重复生成

在项目根目录、已安装 Pillow 的主机环境执行：

```powershell
python tools\generate_font.py
python scripts\preview.py
```

`apps/codex-monitor/tools/preview.py` 直接包含正式 `apps/codex-monitor/target/src/codex_monitor.c` renderer，生成 480x272 RGB565 来源的 `build/preview.ppm` 和查看用 `build/preview.png`。字体改动后的对比文件位于 `build/font-refinement/preview-old.png` 与 `build/font-refinement/preview-new.png`。

百分号专项检查使用：

```powershell
python scripts\font-debug.py
```

输出 `build/font-refinement/glyph-debug.png`，包含 `0123456789%` 以及 `0%`、`8%`、`72%`、`84%`、`92%`、`99%`、`100%`。当前大号 `%` 为 `37x29`、advance `37`，位图包含上下两个环形区域和中间斜线。

ARM 目标构建仍使用 `scripts/target-flags.txt` 中冻结的 ARMv4T/ARM920T、`-mabi=apcs-gnu`、freestanding、`-nostdlib` 和自定义 OABI syscall runtime。字体 refinement 阶段不上传 target、不停止 Qtopia、不修改持久化系统。
