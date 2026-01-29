/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Processor definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_PROCESSOR_H
#define _ASM_M65832_PROCESSOR_H

#include <asm/ptrace.h>
#include <asm/page.h>

/*
 * User space process size: 2GB (0x00000000 - 0x7FFFFFFF)
 * Kernel space starts at PAGE_OFFSET (0x80000000)
 */
#define TASK_SIZE		PAGE_OFFSET
#define TASK_UNMAPPED_BASE	(TASK_SIZE / 3)

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

	/* Saved kernel-mode registers during context switch */
	unsigned long r16, r17, r18, r19, r20, r21, r22, r23;

	/* Frame pointer */
	unsigned long r29;

	/* Return address */
	unsigned long r30;

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
#define start_thread(regs, pc, usp)		\
do {						\
	(regs)->pc = (pc);			\
	(regs)->sp = (usp);			\
	(regs)->status = SR_USER_MODE;		\
} while (0)

/* Free all resources held by a thread. */
static inline void release_thread(struct task_struct *dead_task)
{
}

/* Return saved PC of a blocked thread. */
unsigned long __get_wchan(struct task_struct *p);

#define task_pt_regs(tsk) \
	((struct pt_regs *)(task_stack_page(tsk) + THREAD_SIZE) - 1)

/* Kernel stack size */
#define THREAD_SIZE_ORDER	1
#define THREAD_SIZE		(PAGE_SIZE << THREAD_SIZE_ORDER)

/* CPU initialization */
void cpu_init(void);

/*
 * Prefetch macros - M65832 doesn't have prefetch instructions
 */
#define ARCH_HAS_PREFETCH
static inline void prefetch(const void *x) { }
#define prefetchw(x)	prefetch(x)

/*
 * A macro to define the 'copy_thread' arch function
 */
struct task_struct;
struct pt_regs;
int copy_thread(struct task_struct *p, const struct kernel_clone_args *args);

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

#endif /* _ASM_M65832_PROCESSOR_H */
