/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Stack trace support for the M65832 architecture.
 */

#ifndef _ASM_M65832_STACKTRACE_H
#define _ASM_M65832_STACKTRACE_H

#include <linux/sched.h>

/*
 * Stack frame structure for M65832
 * R29 is frame pointer, R30 is link register
 */
struct stackframe {
	unsigned long fp;	/* R29 - frame pointer */
	unsigned long ra;	/* R30 - return address */
};

extern void dump_stack(void);
extern void show_stack(struct task_struct *task, unsigned long *sp,
		       const char *loglvl);

static inline void start_backtrace(struct stackframe *frame,
				   unsigned long fp, unsigned long pc)
{
	frame->fp = fp;
	frame->ra = pc;
}

extern int unwind_frame(struct task_struct *task, struct stackframe *frame);

#endif /* _ASM_M65832_STACKTRACE_H */
