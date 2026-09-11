TARGET ?= arm-linux-gnueabi
CC := $(TARGET)-gcc

COMMON_CFLAGS := -march=armv4t -mcpu=arm920t -marm -mfloat-abi=soft \
                 -Os -Wall -Wextra -std=c99
STATIC_LDFLAGS := -static
OABI_CFLAGS := -march=armv4t -mcpu=arm920t -marm -mabi=apcs-gnu \
               -mfloat-abi=soft -Os -Wall -Wextra -std=c99 \
               -ffreestanding -fno-builtin -fno-stack-protector
OABI_LDFLAGS := -nostdlib -nodefaultlibs -nostartfiles \
                -Wl,--no-warn-mismatch -Wl,-e,_start

BUILD_DIR := build
APP_TARGET := apps/codex-monitor/target
PLATFORM_RUNTIME := platform/target/runtime
APP_INCLUDE := $(APP_TARGET)/include

.PHONY: all clean target codex-monitor oabi preview status-preview bridge

all: $(BUILD_DIR)/hello $(BUILD_DIR)/fbprobe codex-monitor

oabi: $(BUILD_DIR)/hello-oabi $(BUILD_DIR)/fbprobe-oabi

oabi-ui codex-monitor target: $(BUILD_DIR)/codex-monitor-oabi

preview:
	python apps/codex-monitor/tools/preview.py

status-preview:
	python apps/codex-monitor/tools/status-preview.py

bridge:
	powershell -ExecutionPolicy Bypass -File scripts/build/build-bridge.ps1

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/hello: platform/target/diagnostics/hello.c | $(BUILD_DIR)
	$(CC) $(COMMON_CFLAGS) $(STATIC_LDFLAGS) -o $@ $<

$(BUILD_DIR)/fbprobe: platform/target/diagnostics/fbprobe.c | $(BUILD_DIR)
	$(CC) $(COMMON_CFLAGS) $(STATIC_LDFLAGS) -o $@ $<

$(BUILD_DIR)/hello-oabi: $(PLATFORM_RUNTIME)/oabi_start.S platform/target/diagnostics/hello_oabi.c | $(BUILD_DIR)
	$(CC) $(OABI_CFLAGS) -c $(PLATFORM_RUNTIME)/oabi_start.S -o $(BUILD_DIR)/oabi_start.o
	$(CC) $(OABI_CFLAGS) $(OABI_LDFLAGS) -o $@ $(BUILD_DIR)/oabi_start.o platform/target/diagnostics/hello_oabi.c -lgcc

$(BUILD_DIR)/fbprobe-oabi: $(PLATFORM_RUNTIME)/oabi_start.S platform/target/diagnostics/fbprobe_oabi.c | $(BUILD_DIR)
	$(CC) $(OABI_CFLAGS) -c $(PLATFORM_RUNTIME)/oabi_start.S -o $(BUILD_DIR)/oabi_start.o
	$(CC) $(OABI_CFLAGS) $(OABI_LDFLAGS) -o $@ $(BUILD_DIR)/oabi_start.o platform/target/diagnostics/fbprobe_oabi.c -lgcc

$(BUILD_DIR)/codex-monitor-oabi: $(PLATFORM_RUNTIME)/oabi_start.S $(PLATFORM_RUNTIME)/oabi_runtime.c $(APP_TARGET)/src/codex_monitor.c $(APP_INCLUDE)/font_data.h $(APP_INCLUDE)/ui_layout.h | $(BUILD_DIR)
	$(CC) $(OABI_CFLAGS) -I$(APP_INCLUDE) -c $(PLATFORM_RUNTIME)/oabi_start.S -o $(BUILD_DIR)/oabi_start.o
	$(CC) $(OABI_CFLAGS) $(OABI_LDFLAGS) -I$(APP_INCLUDE) -o $@ $(BUILD_DIR)/oabi_start.o $(PLATFORM_RUNTIME)/oabi_runtime.c $(APP_TARGET)/src/codex_monitor.c -lgcc

clean:
	rm -rf $(BUILD_DIR)
