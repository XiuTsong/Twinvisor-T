
include common.mk
include toolchain.mk

BINARIES_PATH		?= $(ROOT)/out/bin
SOC_TERM_PATH		?= $(ROOT)/soc_term
ARCH ?= arm64

################################################################################
# Linux kernel
################################################################################
LINUX_PATH := $(ROOT)/linux
LINUX_DEFCONFIG_COMMON_ARCH := arm64
LINUX_DEFCONFIG_COMMON_FILES := \
		$(LINUX_PATH)/arch/arm64/configs/defconfig \
		$(CURDIR)/qemu.conf

linux-defconfig: $(LINUX_PATH)/.config

LINUX_FLAGS ?= ARCH=arm64 LOCALVERSION= CROSS_COMPILE="$(CCACHE)$(AARCH64_CROSS_COMPILE)"

linux: linux-defconfig
	$(MAKE) -C $(LINUX_PATH) $(LINUX_FLAGS)
	mkdir -p $(BINARIES_PATH)
	ln -sf $(LINUX_PATH)/arch/arm64/boot/Image $(BINARIES_PATH)

linux-modules: linux
	$(MAKE) -C $(LINUX_PATH) $(LINUX_FLAGS) modules
	$(MAKE) -C $(LINUX_PATH) $(LINUX_FLAGS) INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=$(MODULE_OUTPUT) modules_install

linux-defconfig-clean: linux-defconfig-clean-common

LINUX_CLEAN_COMMON_FLAGS += ARCH=arm64

linux-clean: linux-clean-common

LINUX_CLEANER_COMMON_FLAGS += ARCH=arm64

linux-cleaner: linux-cleaner-common

################################################################################
# Run targets
################################################################################
TARGET_DEPS := linux
TARGET_CLEAN := linux-clean

all: $(TARGET_DEPS)
clean: $(TARGET_CLEAN)

.PHONY: run
# This target enforces updating root fs etc
run: all
	$(MAKE) run-only

QEMU_SMP 	?= 4
QEMU_MEM 	?= 8192
QEMU_VIRT	= true

Image ?= $(BINARIES_PATH)/Image
ROOTFS ?= $(BINARIES_PATH)/rootfs.cpio.gz

#-serial tcp:localhost:54320 -serial tcp:localhost:54321
.PHONY: run-only
run-only:
	ln -sf $(ROOT)/out-br/images/rootfs.cpio.gz $(BINARIES_PATH)/
	#$(call launch-terminal,54320,"Normal World")
	#$(call wait-for-ports,54320)
	$(CURDIR)/qemu-system-aarch64 \
		-s -S \
		-nographic \
		-serial tcp:localhost:44320 \
		-smp 4 \
		-machine virt,secure=on,gic-version=3,virtualization=true \
		-cpu max \
		-d unimp -semihosting-config enable=on,target=native \
		-m 8192 \
		-initrd $(ROOTFS) \
		-kernel $(Image) -no-acpi \
		-append 'cma=1G@6G memmap=256M@2G console=ttyAMA0,38400 keep_bootcon root=/dev/vda2' \
		-device virtio-blk-device,drive=image \
		-drive if=none,id=image,file=$(ROOT)/out/host-vdisk.img,format=raw \
		-device virtio-net-device,netdev=hostnet0,mac=52:54:00:11:33:55 \
		-netdev tap,ifname=tap0,id=hostnet0,script=no \
		$(QEMU_EXTRA_ARGS)
