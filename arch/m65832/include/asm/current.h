/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Current task pointer access for the M65832 architecture.
 *
 * On M65832, we keep:
 *   R24 - pointer to current thread_info
 *   R25 - pointer to current task_struct
 */

#ifndef _ASM_M65832_CURRENT_H
#define _ASM_M65832_CURRENT_H

#ifndef __ASSEMBLY__

#include <linux/compiler.h>

struct task_struct;

/*
 * Get the current task_struct pointer from R25
 */
static __always_inline struct task_struct *get_current(void)
{
	struct task_struct *current_task;

	asm volatile(
		"LD %0, R25"
		: "=r" (current_task)
	);

	return current_task;
}

#define current get_current()

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_CURRENT_H */
