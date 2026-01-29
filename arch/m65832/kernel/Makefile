# SPDX-License-Identifier: GPL-2.0
#
# Makefile for M65832 kernel files
#

extra-y := head.o vmlinux.lds

obj-y := entry.o setup.o process.o traps.o irq.o time.o signal.o ptrace.o
obj-y += sys_m65832.o

obj-$(CONFIG_MODULES)		+= module.o
obj-$(CONFIG_M65832_EARLY_PRINTK)	+= early_printk.o
