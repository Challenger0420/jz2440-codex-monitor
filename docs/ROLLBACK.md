# 回滚方案（仅设计）

回滚目标是恢复到部署前的 Qtopia 启动链，不重写 NAND。

## 正常回滚

1. 停止 Windows Bridge，确认不再占用 COM。
2. 通过 `<CQMQUIT>` 或 wrapper 的超时路径退出 Monitor。
3. 运行部署前已验证的 Qtopia 恢复命令：`/bin/qpe.sh &`。
4. 删除未来新增的 `/opt/codex-monitor/` 文件和 wrapper（仅在未来部署确实创建后执行）。
5. 删除未来新增的自启动配置，恢复部署前的 init 文件备份。
6. reboot 并通过串口确认原 U-Boot、kernel、YAFFS2、Qtopia 正常。

## 失败恢复

如果 Monitor 崩溃，保留 shell；如果 wrapper 失效，通过原 console 手动启动 `/bin/qpe.sh &`。不要用 `nand erase/write`、`dd`、格式化或 `saveenv` 作为回滚手段。

## 证据要求

正式部署前必须保存部署前的启动链、文件清单、校验和及 init 配置；本轮没有创建这些持久化文件，也没有执行回滚操作。

