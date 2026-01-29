/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Context switch definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_SWITCH_TO_H
#define _ASM_M65832_SWITCH_TO_H

#include <asm/processor.h>
#include <asm/ptrace.h>

struct task_struct;

/*
 * Low-level context switch function (in assembly)
 */
extern struct task_struct *__switch_to(struct task_struct *prev,
				       struct task_struct *next);

/*
 * Context switch macro
 *
 * switch_to(prev, next, last) switches from 'prev' to 'next', and
 * stores the task that was running before 'next' in 'last'.
 */
#define switch_to(prev, next, last)					\
do {									\
	((last) = __switch_to((prev), (next)));				\
} while (0)

/*
 * Prepare to switch - save FPU state if needed
 */
#define prepare_to_switch()	do { } while (0)

/*
 * Finish architecture-specific switch handling
 */
#define finish_arch_switch(prev)	do { } while (0)

#endif /* _ASM_M65832_SWITCH_TO_H */
