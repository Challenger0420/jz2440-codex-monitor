# 未来部署计划（仅设计）

> 本文是早期 Codex Monitor 单应用部署记录。平台化后的通用入口位于
> `scripts/deploy/`；本文不代表已经启用开机自启。

目标目录：`/opt/codex-monitor/`，预计包含 `codex-monitor`、`run.sh`、`VERSION`。Windows 侧为 `CodexJZBridge.exe`。

建议流程：

1. Linux 正常启动并保留可恢复 shell。
2. wrapper 记录 Qtopia 当前状态，受控暂停 Qtopia 后启动 Monitor。
3. Bridge 自动发现 PL2303，连接失败只重试，不发送伪造 quota。
4. Monitor 正常收到 `<CQMQUIT>`、Ctrl-C、崩溃或串口断开时释放资源。
5. wrapper 恢复 qpe/qss/quicklauncher；恢复失败时留下日志并保留人工 shell 恢复路径。
6. reboot 后仍走原系统启动链，部署不能依赖临时 `/tmp` 状态。

本轮不创建目标目录、不改 `/etc`、init、bootargs、rootfs、NAND 或 Windows Task Scheduler。

