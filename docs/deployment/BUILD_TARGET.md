# Target 构建冻结记录

## 已验证 ABI

目标是 S3C2440A / ARM920T / ARMv4T。Monitor 使用 freestanding OABI syscall runtime，不能替换成现代 EABI5 libc 路线；此前已验证现代 EABI5 静态程序在目标上会触发 Illegal instruction。

固定参数：

```text
-march=armv4t -mcpu=arm920t -marm -mabi=apcs-gnu
-mfloat-abi=soft -Os -Wall -Wextra -std=c99
-ffreestanding -fno-builtin -fno-stack-protector
-nostdlib -nodefaultlibs -nostartfiles
-Wl,--no-warn-mismatch -Wl,-e,_start
```

入口是 `platform/target/runtime/oabi_start.S` 的 `_start`；链接时使用 `platform/target/runtime/oabi_runtime.c`、`apps/codex-monitor/target/src/codex_monitor.c` 和 compiler support library `-lgcc`。OABI syscall number 和调用约定集中在 `platform/target/runtime/oabi_runtime.c`。

## 可重复入口

在安装了同一 ARM GCC 的 PowerShell 中执行：

```powershell
$env:JZ2440_ARM_GCC = "C:\toolchain\bin\arm-linux-gnueabi-gcc.exe"
powershell -ExecutionPolicy Bypass -File scripts\build\build-target.ps1
```

Linux 等价入口：

```bash
bash scripts/build/build-target.sh
```

两套脚本共同读取 `scripts/build/target-flags.txt`，因此 CFLAGS/LDFLAGS 不在两个脚本中分叉。脚本只生成 `build/codex-monitor-oabi`，并在构建前删除该目标的本地中间文件；不会访问开发板。

目标检查使用 ARM 交叉工具链自带程序，而不是 Windows 原生 binutils：

```bash
bash scripts/build/inspect-target.sh build/codex-monitor-oabi
```

该检查覆盖 `file`、`readelf -h/-A/-l/-d`、`objdump -f/-d`，并扫描已知 ARMv5+/ARMv6+ 指令助记符。

或在支持 Make 的环境执行：

```text
make target
```

脚本不会访问开发板，只生成 `build/codex-monitor-oabi`。

## 冻结的完整编译/链接命令

```text
arm-linux-gnueabi-gcc -march=armv4t -mcpu=arm920t -marm -mabi=apcs-gnu -mfloat-abi=soft -Os -Wall -Wextra -std=c99 -ffreestanding -fno-builtin -fno-stack-protector -I apps/codex-monitor/target/include -c platform/target/runtime/oabi_start.S -o build/oabi_start.o
arm-linux-gnueabi-gcc -march=armv4t -mcpu=arm920t -marm -mabi=apcs-gnu -mfloat-abi=soft -Os -Wall -Wextra -std=c99 -ffreestanding -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs -nostartfiles -Wl,--no-warn-mismatch -Wl,-e,_start -I apps/codex-monitor/target/include -o build/codex-monitor-oabi build/oabi_start.o platform/target/runtime/oabi_runtime.c apps/codex-monitor/target/src/codex_monitor.c -lgcc
```

## 当前 ELF 摘要

现有已验证 artifact：`build/codex-monitor-oabi`，9572 bytes。

```text
file: ELF 32-bit LSB executable, ARM, statically linked, not stripped
type: EXEC
machine: ARM
entry: 0x000106bc
ELF flags: 0x600, software FP
Tag_CPU_name: 4T
Tag_CPU_arch: v4T
Tag_ARM_ISA_use: Yes
Tag_THUMB_ISA_use: Thumb-1
```

`readelf -A` 确认 CPU 架构为 v4T；当前 Windows `objdump` 无法识别该 ARM ELF 进行反汇编，因此 ARM 指令级审计仍应在交叉工具链环境运行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build\inspect-target.ps1
```

## 当前验证状态

工具链已恢复为 Ubuntu 官方包：`gcc-11-arm-linux-gnueabi 11.4.0-1ubuntu1~22.04.3cross1` 和 `binutils-arm-linux-gnueabi 2.38-4ubuntu2.12`。使用相同源码和 `build-target.sh` 连续 clean build 两次，Build A/B SHA256 完全一致。ARM 交叉 `readelf`/`objdump` 确认新 ELF 为 ARMv4T、无 dynamic section，未发现列出的 ARMv5+/ARMv6+ 指令。详细构建元数据位于 `build/rc/BUILD_INFO.txt`。

Build Gate 记录：

```bash
mkdir -p build/reference
cp build/codex-monitor-oabi build/reference/codex-monitor-oabi-pre-rc
bash scripts/build/build-target.sh                    # Build A
sha256sum build/codex-monitor-oabi > build/sha256-build-1.txt
bash scripts/build/inspect-target.sh build/codex-monitor-oabi
bash scripts/build/build-target.sh                    # Build B
sha256sum build/codex-monitor-oabi > build/sha256-build-2.txt
diff -u build/sha256-build-1.txt build/sha256-build-2.txt
```

当前 target release candidate：`build/rc/codex-monitor-oabi`。该文件只用于后续审核，尚未上传开发板。
