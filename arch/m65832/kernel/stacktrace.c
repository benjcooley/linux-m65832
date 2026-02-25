// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Stack trace support for the M65832 architecture.
 *
 * M65832 calling convention:
 *   B  = frame pointer (callee-saved, set via PHB32/TSPB prologue)
 *   JSR/RTS = call/return via stack (return address pushed on stack)
 *
 * Frame pointer chain walking (like ARM/RISC-V/PowerPC):
 *   After PHB32; [alloc N locals;] TSPB, B = SP.
 *   Due to 65816 push semantics (store then SP-=4), old_B is at
 *   B+N+1 and the return address (from caller's JSR) at B+N+5.
 *   We scan upward from B+1 looking for a (valid_stack_ptr,
 *   kernel_text_addr) pair to handle variable N.
 */

#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/kallsyms.h>
#include <linux/stacktrace.h>

#include <asm/ptrace.h>
#include <asm/stacktrace.h>
#include <asm/page.h>

/*
 * Unwind one frame using frame pointer chain scan.
 * Scans upward from fp+1 looking for (old_B, return_addr) pair.
 * Returns 0 on success, -1 on failure.
 */
int unwind_frame(struct task_struct *task, struct stackframe *frame)
{
	unsigned long fp = frame->fp;
	unsigned long stack_end;
	int off;

	if (!fp || fp < PAGE_OFFSET)
		return -1;

	if (task)
		stack_end = (unsigned long)task_stack_page(task) + THREAD_SIZE;
	else
		stack_end = ALIGN(fp, THREAD_SIZE);

	for (off = 1; off <= 256 && (fp + off + 7) < stack_end; off += 4) {
		unsigned long cfp = *(unsigned long *)(fp + off);
		unsigned long cra = *(unsigned long *)(fp + off + 4);

		if (cfp > fp && cfp < stack_end &&
		    cfp >= PAGE_OFFSET &&
		    __kernel_text_address(cra)) {
			frame->fp = cfp;
			frame->ra = cra;
			return 0;
		}
	}

	return -1;
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
		/* Start from B register (frame pointer) */
		start_backtrace(&frame, regs->b, regs->pc);
	} else if (task == current) {
		start_backtrace(&frame, 0, 0);
		return;
	} else {
		start_backtrace(&frame, task->thread.ksp, 0);
	}

	/* Report PC first */
	if (frame.ra) {
		if (!consume_fn(cookie, frame.ra))
			return;
	}

	while (1) {
		if (unwind_frame(task, &frame))
			break;
		if (!frame.ra)
			break;
		if (!consume_fn(cookie, frame.ra))
			break;
	}
}

/*
 * Show stack - walk frame pointer chain
 */
void show_stack(struct task_struct *task, unsigned long *sp, const char *loglvl)
{
	unsigned long fp;
	unsigned long stack_end;
	int count = 0;

	if (!task)
		task = current;

	printk("%sCall Trace:\n", loglvl);

	if (!sp) {
		if (task == current) {
			printk("%s  (stack trace not available for current task)\n", loglvl);
			return;
		}
		sp = (unsigned long *)task->thread.ksp;
	}

	stack_end = (unsigned long)task_stack_page(task) + THREAD_SIZE;

	/* Walk frame pointer chain */
	fp = (unsigned long)sp;
	while (fp && fp >= PAGE_OFFSET && fp < stack_end && count < 32) {
		unsigned long old_fp = 0;
		unsigned long ra = 0;
		int off;

		for (off = 1; off <= 256 && (fp + off + 7) < stack_end; off += 4) {
			unsigned long cfp = *(unsigned long *)(fp + off);
			unsigned long cra = *(unsigned long *)(fp + off + 4);

			if (cfp > fp && cfp < stack_end &&
			    cfp >= PAGE_OFFSET &&
			    __kernel_text_address(cra)) {
				old_fp = cfp;
				ra = cra;
				break;
			}
		}

		if (!old_fp)
			break;

		/*
		 * HACK(m65832-boot): Print raw addresses instead of using
		 * %pS (kallsyms_lookup).  Symbol lookup walks the entire
		 * compressed kallsyms table byte-by-byte and takes millions
		 * of emulated cycles per symbol.
		 * TODO: Revert to %pS when the compiler is faster or when
		 * we add a binary-search kallsyms lookup.
		 */
		printk("%s [<%08lx>]\n", loglvl, ra);
		fp = old_fp;
		count++;
	}

	if (!count)
		printk("%s  (no return addresses found)\n", loglvl);
}
