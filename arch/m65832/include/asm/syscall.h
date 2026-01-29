/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Syscall handling definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_SYSCALL_H
#define _ASM_M65832_SYSCALL_H

#include <linux/sched.h>
#include <linux/err.h>
#include <asm/ptrace.h>
#include <asm/unistd.h>

/*
 * Syscall calling convention:
 *   R0 - syscall number
 *   R1-R6 - arguments 1-6
 *   R0 - return value (negative for error)
 */

extern void *sys_call_table[];

static inline int syscall_get_nr(struct task_struct *task, struct pt_regs *regs)
{
	return regs->r0;
}

static inline void syscall_rollback(struct task_struct *task,
				    struct pt_regs *regs)
{
	regs->r0 = regs->orig_r0;
}

static inline long syscall_get_error(struct task_struct *task,
				     struct pt_regs *regs)
{
	unsigned long error = regs->r0;

	return IS_ERR_VALUE(error) ? error : 0;
}

static inline long syscall_get_return_value(struct task_struct *task,
					    struct pt_regs *regs)
{
	return regs->r0;
}

static inline void syscall_set_return_value(struct task_struct *task,
					    struct pt_regs *regs,
					    int error, long val)
{
	regs->r0 = error ?: val;
}

static inline void syscall_get_arguments(struct task_struct *task,
					 struct pt_regs *regs,
					 unsigned long *args)
{
	args[0] = regs->r1;
	args[1] = regs->r2;
	args[2] = regs->r3;
	args[3] = regs->r4;
	args[4] = regs->r5;
	args[5] = regs->r6;
}

static inline int syscall_get_arch(struct task_struct *task)
{
	return AUDIT_ARCH_M65832;
}

#endif /* _ASM_M65832_SYSCALL_H */
