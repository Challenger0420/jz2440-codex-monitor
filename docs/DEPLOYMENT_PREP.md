# Deployment preparation

本阶段只准备长期运行所需的接口和恢复设计，不创建板端目录、不上传脚本、不设置开机自启。

## 已确认的串口映射

USB-COM1 → JZ2440 UART0 → `/dev/s3c2410_serial0`。依据是：

1. USB-COM1 的 shell 在 COM6 上实际可交互；
2. kernel command line 使用 `console=ttySAC0`；
3. `/proc/tty/driver/serial` 只有 UART 0 具有有效 MMIO/IRQ 信息；
4. `/dev/ttySAC*` 不存在；`/dev/ttyS0` 虽存在但 direct backend 的 `TCGETS` 配置失败；实际可打开并接收的 UART0 节点是 `/dev/s3c2410_serial0`。

正式部署前仍应在停止 Qtopia、shell 前台等待的受控窗口中用 direct backend 做实测；不能仅根据节点名称猜测。

## Monitor 输入模式

```text
调试：  codex-monitor --stdin
  正式：  codex-monitor --serial /dev/s3c2410_serial0
```

两种模式共享同一个 CQM1 parser 和 framebuffer renderer。direct 模式使用现有 freestanding OABI runtime 的 `open`、`ioctl`、`fcntl`、`read`、`close`：

- 115200 baud
- 8 data bits
- 1 stop bit
- no parity
- no hardware/software flow control

direct backend 保存并恢复原始 termios，避免 `<CQMQUIT>` 后 shell 留在 raw/non-echo 状态。

## Windows Bridge

显式 `--port COM6` 时使用用户指定端口；未指定时按 `USB\\VID_067B&PID_2303` 在 WMI PnP 信息中查找唯一 COM 端口。零个候选会在下一周期重试，多个候选会拒绝猜测并等待显式端口。SerialPort 打开、写入或 provider 失败均释放旧连接并在下一周期重试。

## 草案脚本

- `scripts/start-monitor.sh`：草案，不部署。幂等检查 monitor，临时停止 Qtopia，再执行 `--serial /dev/s3c2410_serial0`。
- `scripts/stop-monitor.sh`：草案，不部署。优先向 direct serial 发送 `<CQMQUIT>`，执行 `/bin/qpe.sh &`，必要时按已验证环境启动 `today`。

最终脚本还需补充 PID/状态判断、超时处理和日志路径设计；本阶段不写 `/etc`、`/opt`、`/home`，不修改 init 或启动链。

## 验收顺序

1. 上传到 `/tmp` 的临时 RC；
2. direct serial 连续至少五个真实 refresh；
3. Bridge 拔插/COM 号变化后自动重新枚举；
4. stale → reconnect → normal；
5. `<CQMQUIT>` 后恢复 Qtopia/Today；
6. 全部通过后再单独评审 FINAL_DEPLOYMENT。
