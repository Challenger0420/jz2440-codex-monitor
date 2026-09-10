TARGET ?= arm-linux-gnueabi
CC := $(TARGET)-gcc

COMMON_CFLAGS := -march=armv4t -mcpu=arm920t -marm -mfloat-abi=soft \
                 -Os -Wall -Wextra -std=c99
STATIC_LDFLAGS := -static

BUILD_DIR := build

.PHONY: all clean target preview

all: $(BUILD_DIR)/hello $(BUILD_DIR)/fbprobe $(BUILD_DIR)/codex-monitor

OABI_CFLAGS := -march=armv4t -mcpu=arm920t -marm -mabi=apcs-gnu \
               -mfloat-abi=soft -Os -Wall -Wextra -std=c99 \
               -ffreestanding -fno-builtin -fno-stack-protector
OABI_LDFLAGS := -nostdlib -nodefaultlibs -nostartfiles \
                -Wl,--no-warn-mismatch -Wl,-e,_start

oabi: $(BUILD_DIR)/hello-oabi $(BUILD_DIR)/fbprobe-oabi

oabi-ui: $(BUILD_DIR)/codex-monitor-oabi

target: oabi-ui

preview:
	python scripts/preview.py

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/hello: src/hello.c | $(BUILD_DIR)
	$(CC) $(COMMON_CFLAGS) $(STATIC_LDFLAGS) -o $@ $<

$(BUILD_DIR)/fbprobe: src/fbprobe.c | $(BUILD_DIR)
	$(CC) $(COMMON_CFLAGS) $(STATIC_LDFLAGS) -o $@ $<

$(BUILD_DIR)/codex-monitor: src/codex_monitor.c | $(BUILD_DIR)
	$(CC) $(COMMON_CFLAGS) $(STATIC_LDFLAGS) -o $@ $<

$(BUILD_DIR)/hello-oabi: src/oabi_start.S src/hello_oabi.c | $(BUILD_DIR)
	$(CC) $(OABI_CFLAGS) -c src/oabi_start.S -o $(BUILD_DIR)/oabi_start.o
	$(CC) $(OABI_CFLAGS) $(OABI_LDFLAGS) -o $@ $(BUILD_DIR)/oabi_start.o src/hello_oabi.c -lgcc

$(BUILD_DIR)/fbprobe-oabi: src/oabi_start.S src/fbprobe_oabi.c | $(BUILD_DIR)
	$(CC) $(OABI_CFLAGS) -c src/oabi_start.S -o $(BUILD_DIR)/oabi_start.o
	$(CC) $(OABI_CFLAGS) $(OABI_LDFLAGS) -o $@ $(BUILD_DIR)/oabi_start.o src/fbprobe_oabi.c -lgcc

$(BUILD_DIR)/codex-monitor-oabi: src/oabi_start.S src/oabi_runtime.c src/codex_monitor.c | $(BUILD_DIR)
	$(CC) $(OABI_CFLAGS) -c src/oabi_start.S -o $(BUILD_DIR)/oabi_start.o
	$(CC) $(OABI_CFLAGS) $(OABI_LDFLAGS) -o $@ $(BUILD_DIR)/oabi_start.o src/oabi_runtime.c src/codex_monitor.c -lgcc

clean:
	rm -rf $(BUILD_DIR)
