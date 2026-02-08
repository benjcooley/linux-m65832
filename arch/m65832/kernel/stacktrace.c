// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Stack trace support for the M65832 architecture.
 *
 * Per the M65832 ABI:
 *   B = frame pointer (callee-saved)
 *   R30 = link register (return address)
 */

#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/kallsyms.h>
#include <linux/stacktrace.h>

#include <asm/ptrace.h>
#include <asm/stacktrace.h>

/*
 * Unwind one frame
 * Returns 0 on success, -1 on failure
 *
 * Frame layout (B-relative):
 *   [B+0] = saved previous B (frame pointer chain)
 *   [B+4] = saved return address
 */
int unwind_frame(struct task_struct *task, struct stackframe *frame)
{
	unsigned long fp = frame->fp;

	/* Check for valid frame pointer */
	if (!fp || fp & 3)
		return -1;

	/* Read the previous frame pointer and return address */
	frame->fp = *(unsigned long *)(fp);
	frame->ra = *(unsigned long *)(fp + 4);

	/* Validate the new return address */
	if (!frame->ra)
		return -1;

	return 0;
}

/*
 * Walk the stack and call consume_fn for each frame.
 * Modern kernel interface: arch_stack_walk.
 */
void arch_stack_walk(stack_trace_consume_fn consume_fn, void *cookie,
		     struct task_struct *task, struct pt_regs *regs)
{
	struct stackframe frame;

	if (regs) {
		start_backtrace(&frame, regs->b, regs->r30);
	} else if (task == current) {
		/* For current task, use a stub - real unwinding needs
		 * the actual B and R30 values from inline asm.
		 * TODO: Use proper inline asm once B constraint works.
		 */
		start_backtrace(&frame, 0, 0);
		return;
	} else {
		/* For sleeping tasks, get saved context */
		start_backtrace(&frame, task->thread.b, 0);
	}

	while (1) {
		if (!frame.ra)
			break;
		if (!consume_fn(cookie, frame.ra))
			break;
		if (unwind_frame(task, &frame))
			break;
	}
}

/*
 * Show stack for debugging
 */
void show_stack(struct task_struct *task, unsigned long *sp, const char *loglvl)
{
	struct stackframe frame;
	int count = 0;

	if (!task)
		task = current;

	if (task == current) {
		/* Simplified: just show current PC */
		printk("%sCall Trace:\n", loglvl);
		printk("%s  (stack trace not available for current task)\n", loglvl);
		return;
	}

	start_backtrace(&frame, task->thread.b, 0);

	printk("%sCall Trace:\n", loglvl);

	while (count < 64) {
		if (!frame.ra)
			break;
		printk("%s [<%08lx>] %pS\n", loglvl, frame.ra, (void *)frame.ra);
		if (unwind_frame(task, &frame))
			break;
		count++;
	}
}
