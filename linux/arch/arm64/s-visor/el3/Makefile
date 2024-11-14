# SPDX-License-Identifier: GPL-2.0
#
# Makefile for secure hypervisor module: el3_runtime
# Port from trusted-firmware-a
#

# Add -fno-stack-protector flag for all files in s-visor directory
ccflags-y := -fno-stack-protector
asflags-y := -fno-stack-protector

obj-y += context.o
obj-y += context_mgmt.o
obj-y += runtime_exception.o
obj-y += runtime_svc.o
obj-y += titanium_main.o
obj-y += platform.o
obj-y += cpu_data_array.o
obj-y += smc.o
