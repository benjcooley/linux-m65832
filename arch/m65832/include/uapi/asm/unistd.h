/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * M65832 Linux
 *
 * System call numbers for userspace.
 *
 * We use the generic syscall table (asm-generic/unistd.h) which provides
 * a modern, clean syscall interface.
 */

#ifndef _UAPI_ASM_M65832_UNISTD_H
#define _UAPI_ASM_M65832_UNISTD_H

/*
 * Use the generic syscall table
 * This gives us ~440 syscalls with consistent numbering
 */
#include <asm-generic/unistd.h>

#endif /* _UAPI_ASM_M65832_UNISTD_H */
