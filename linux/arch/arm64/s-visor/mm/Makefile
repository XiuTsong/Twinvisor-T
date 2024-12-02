# SPDX-License-Identifier: GPL-2.0
#
# Makefile for secure hypervisor module: mm
#

# Add -fno-stack-protector flag for all files in s-visor directory
ccflags-y := -fno-stack-protector
asflags-y := -fno-stack-protector

obj-y += tlsf.o \
	 	 page_allocator.o \
		 buddy_allocator.o \
		 mm.o \
		 spt_info.o \
		 sec_mmu.o \
		 stage2_mmu.o \
		 sec_fixmap.o