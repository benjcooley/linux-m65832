# SPDX-License-Identifier: GPL-2.0
#
# Makefile for M65832 boot files
#

targets := vmlinux.bin

OBJCOPYFLAGS_vmlinux.bin := -O binary

$(obj)/vmlinux.bin: vmlinux FORCE
	$(call if_changed,objcopy)
