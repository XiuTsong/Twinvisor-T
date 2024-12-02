# SPDX-License-Identifier: GPL-2.0
#
# Makefile for secure hypervisor module: virt
#

# Add -fno-stack-protector flag for all files in s-visor directory
ccflags-y := -fno-stack-protector
asflags-y := -fno-stack-protector

obj-y += vm.o
obj-y += guest_helpers.o
obj-y += kvm_relay.o
obj-y += sec_vgic.o
obj-y += sec_emulate.o
obj-y += sec_handler.o
