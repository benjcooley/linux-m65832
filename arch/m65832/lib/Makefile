# SPDX-License-Identifier: GPL-2.0
#
# Makefile for M65832 library files
#

lib-y := delay.o memcpy.o memset.o bitops.o

ifeq ($(CONFIG_M65832),y)
CFLAGS_delay.o += -O0
endif
