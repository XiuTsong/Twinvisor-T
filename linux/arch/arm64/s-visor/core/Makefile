# SPDX-License-Identifier: GPL-2.0
#
# Makefile for secure hypervisor module: core
#

# Add -fno-stack-protector flag for all files in s-visor directory
ccflags-y := -fno-stack-protector
asflags-y := -fno-stack-protector

obj-y += guest-dispatcher.o
obj-y += exception.o
obj-y += svisor-main.o
obj-y += guest-entry.o