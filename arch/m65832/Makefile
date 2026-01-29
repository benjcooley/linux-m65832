# SPDX-License-Identifier: GPL-2.0
#
# arch/m65832/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#

KBUILD_DEFCONFIG := defconfig

OBJCOPYFLAGS	:= -O binary -R .note -R .comment -S
LDFLAGS_vmlinux	:=

# Use LLVM's LLD linker
KBUILD_LDFLAGS	+= --gc-sections

# M65832-specific compiler flags
# -ffixed-r24: Reserve R24 for thread_info pointer
# -ffixed-r25: Reserve R25 for current task pointer
KBUILD_CFLAGS	+= -pipe -ffreestanding
KBUILD_CFLAGS	+= -fno-common
KBUILD_AFLAGS	+= -D__ASSEMBLY__

# M65832 doesn't have FPU - no float flags needed

# Architecture-specific includes
KBUILD_CPPFLAGS	+= -I$(srctree)/arch/m65832/include

# Default to all target
all: vmlinux

boot	:= arch/m65832/boot
core-y	+= arch/m65832/kernel/
core-y	+= arch/m65832/mm/
libs-y	+= arch/m65832/lib/

# Boot targets
PHONY += vmlinux.bin

vmlinux.bin: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

archclean:
	$(Q)$(MAKE) $(clean)=$(boot)

define archhelp
  echo '  vmlinux.bin     - Raw binary kernel image'
endef
