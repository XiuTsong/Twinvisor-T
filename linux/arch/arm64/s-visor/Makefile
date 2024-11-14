# SPDX-License-Identifier: GPL-2.0
#
# Makefile for secure hypervisor module
#

# Add -fno-stack-protector flag for all files in s-visor directory
ccflags-y := -fno-stack-protector
asflags-y := -fno-stack-protector

obj-y += n-visor.o

obj-y += common/
obj-y += sched/
obj-y += core/
obj-y += mm/
obj-y += virt/
obj-y += el3/
