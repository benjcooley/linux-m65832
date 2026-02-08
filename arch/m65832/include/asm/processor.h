/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Processor definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_PROCESSOR_H
#define _ASM_M65832_PROCESSOR_H

/*
 * User space process size: 2GB (0x00000000 - 0x7FFFFFFF)
 * Kernel space starts at PAGE_OFFSET (0x80000000)
 */
#define TASK_SIZE		PAGE_OFFSET
#define TASK_SIZE_MAX		TASK_SIZE
#define TASK_UNMAPPED_BASE	(TASK_SIZE / 3)

/* Kernel stack size */
#define THREAD_SIZE_ORDER	1
#define THREAD_SIZE		(PAGE_SIZE << THREAD_SIZE_ORDER)

#ifndef __ASSEMBLY__

#include <asm/ptrace.h>
#include <asm/page.h>

/*
 * Default implementation of macro that returns current instruction pointer
 */
#define current_text_addr()	({ __label__ _l; _l: &&_l; })

/*
 * CPU-specific state for each task
 */
struct thread_struct {
	/* Kernel stack pointer */
	unsigned long ksp;

	/* Saved kernel-mode callee-saved registers during context switch */
	unsigned long r16, r17, r18, r19, r20, r21, r22, r23;

	/* B register - frame pointer (callee-saved) */
	unsigned long b;

	/* User-mode FPU state (if FPU enabled) */
#ifdef CONFIG_M65832_FPU
	unsigned long fpu_state[32];	/* F0-F15 as 64-bit values */
	unsigned long fpsr;		/* FPU status register */
#endif

	/* Fault information */
	unsigned long fault_address;
	unsigned long fault_code;
};

#define INIT_THREAD { }

/*
 * Do necessary setup to start up a newly executed thread.
 */
#define start_thread(regs, new_pc, usp)		\
do {						\
	(regs)->pc = (new_pc);			\
	(regs)->sp = (usp);			\
	(regs)->status = SR_USER_MODE;		\
} while (0)

/* release_thread is provided by kernel/exit.c */

/* Return saved PC of a blocked thread. */
unsigned long __get_wchan(struct task_struct *p);

#define task_pt_regs(tsk) \
	((struct pt_regs *)(task_stack_page(tsk) + THREAD_SIZE) - 1)

/*
 * User-mode stack pointer and instruction pointer for a task
 */
#define KSTK_ESP(tsk)	(task_pt_regs(tsk)->sp)
#define KSTK_EIP(tsk)	(task_pt_regs(tsk)->pc)

/* CPU initialization */
void cpu_init(void);

/*
 * Prefetch macros - M65832 doesn't have prefetch instructions
 */
#define ARCH_HAS_PREFETCH
static inline void prefetch(const void *x) { }
#define prefetchw(x)	prefetch(x)

/* copy_thread is declared in linux/sched/task.h */

/*
 * Stack pointer alignment - 4 bytes for M65832
 */
#define STACK_TOP		TASK_SIZE
#define STACK_TOP_MAX		STACK_TOP
#define STACK_ALIGN		4

/*
 * Idle loop
 */
void cpu_idle(void);

/*
 * cpu_relax - hint to the CPU that we're in a busy-wait loop
 */
static inline void cpu_relax(void)
{
	barrier();
}

#define cpu_relax cpu_relax

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_PROCESSOR_H */
