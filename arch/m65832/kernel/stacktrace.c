// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Stack trace support for the M65832 architecture.
 */

#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/stacktrace.h>
#include <linux/kallsyms.h>

#include <asm/stacktrace.h>
#include <asm/ptrace.h>

/*
 * Unwind one frame
 * Returns 0 on success, -1 on failure
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
 * Walk the stack and call fn for each frame
 */
static void walk_stackframe(struct task_struct *task, struct stackframe *frame,
			    bool (*fn)(void *data, unsigned long addr),
			    void *data)
{
	while (1) {
		if (!fn(data, frame->ra))
			break;
		if (unwind_frame(task, frame))
			break;
	}
}

/*
 * Save stack trace
 */
static bool save_trace(void *data, unsigned long addr)
{
	struct stack_trace *trace = data;

	if (trace->skip > 0) {
		trace->skip--;
		return true;
	}

	if (trace->nr_entries < trace->max_entries) {
		trace->entries[trace->nr_entries++] = addr;
		return true;
	}

	return false;
}

void save_stack_trace_tsk(struct task_struct *task, struct stack_trace *trace)
{
	struct stackframe frame;

	if (task == current) {
		/* Get current frame */
		register unsigned long fp asm("r29");
		register unsigned long ra asm("r30");
		start_backtrace(&frame, fp, ra);
	} else {
		/* Get saved frame from task */
		start_backtrace(&frame, task->thread.r29, task->thread.r30);
	}

	walk_stackframe(task, &frame, save_trace, trace);
}
EXPORT_SYMBOL_GPL(save_stack_trace_tsk);

void save_stack_trace(struct stack_trace *trace)
{
	save_stack_trace_tsk(current, trace);
}
EXPORT_SYMBOL_GPL(save_stack_trace);

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
		register unsigned long fp asm("r29");
		register unsigned long ra asm("r30");
		start_backtrace(&frame, fp, ra);
	} else {
		start_backtrace(&frame, task->thread.r29, task->thread.r30);
	}

	printk("%sCall Trace:\n", loglvl);

	while (count < 64) {
		printk("%s [<%08lx>] %pS\n", loglvl, frame.ra, (void *)frame.ra);
		if (unwind_frame(task, &frame))
			break;
		count++;
	}
}
