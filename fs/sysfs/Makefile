# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the sysfs virtual filesystem
#

obj-y		:= file.o dir.o symlink.o mount.o group.o

ifeq ($(CONFIG_M65832),y)
# Bring-up workaround: avoid kobject/sysfs dir corruption in optimized code.
CFLAGS_dir.o += -O0
endif
