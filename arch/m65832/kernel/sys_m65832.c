// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Architecture-specific system calls for the M65832.
 */

#include <linux/syscalls.h>
#include <linux/signal.h>
#include <linux/unistd.h>

#include <asm/syscall.h>
#include <asm/ptrace.h>

/*
 * M65832 doesn't need any special arch-specific syscalls currently.
 * We use the generic syscall table from asm-generic/unistd.h.
 *
 * If architecture-specific syscalls are needed in the future,
 * they would be added here.
 */

/*
 * System call table
 */
#undef __SYSCALL
#define __SYSCALL(nr, call)	[nr] = (call),

void *sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls - 1] = sys_ni_syscall,
#include <asm/unistd.h>
};
