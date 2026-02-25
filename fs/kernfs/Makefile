# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the kernfs pseudo filesystem
#

obj-y		:= mount.o inode.o dir.o file.o symlink.o

ifeq ($(CONFIG_M65832),y)
CFLAGS_dir.o += -O0
endif

