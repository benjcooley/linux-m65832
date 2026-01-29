/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Thread information structure for the M65832 architecture.
 */

#ifndef _ASM_M65832_THREAD_INFO_H
#define _ASM_M65832_THREAD_INFO_H

#include <asm/page.h>
#include <asm/processor.h>

/*
 * Thread information flags (stored in thread_info.flags)
 */
#define TIF_SYSCALL_TRACE	0	/* syscall trace active */
#define TIF_NOTIFY_RESUME	1	/* callback before returning to user */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
#define TIF_SINGLESTEP		4	/* restore singlestep on return to usermode */
#define TIF_NOTIFY_SIGNAL	5	/* signal notifications exist */
#define TIF_RESTORE_SIGMASK	9	/* restore signal mask in do_signal() */
#define TIF_MEMDIE		18	/* OOM killer killed process */

#define _TIF_SYSCALL_TRACE	(1 << TIF_SYSCALL_TRACE)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_SIGPENDING		(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1 << TIF_NEED_RESCHED)
#define _TIF_SINGLESTEP		(1 << TIF_SINGLESTEP)
#define _TIF_NOTIFY_SIGNAL	(1 << TIF_NOTIFY_SIGNAL)

/* Work to do on any return to user mode */
#define _TIF_WORK_MASK		(_TIF_SIGPENDING | _TIF_NOTIFY_RESUME | \
				 _TIF_NEED_RESCHED | _TIF_NOTIFY_SIGNAL)

/* Work to do on interrupt/exception return */
#define _TIF_ALLWORK_MASK	(_TIF_WORK_MASK | _TIF_SYSCALL_TRACE)

#ifndef __ASSEMBLY__

#include <asm/types.h>

/*
 * Per-thread information structure.
 * This is stored at the bottom of the kernel stack for each thread.
 */
struct thread_info {
	unsigned long		flags;		/* low level flags */
	int			preempt_count;	/* 0 => preemptable, <0 => BUG */
	mm_segment_t		addr_limit;	/* thread address space limit */
	int			cpu;		/* current CPU */
};

/*
 * Initial thread info structure.
 */
#define INIT_THREAD_INFO(tsk)			\
{						\
	.flags		= 0,			\
	.preempt_count	= INIT_PREEMPT_COUNT,	\
	.addr_limit	= KERNEL_DS,		\
}

/*
 * Get the thread_info for the current thread.
 * On M65832, we keep the thread_info pointer in R24.
 */
static inline struct thread_info *current_thread_info(void)
{
	struct thread_info *ti;

	/*
	 * The thread_info is stored at the base of the kernel stack.
	 * We keep a pointer to it in R24 for fast access.
	 */
	asm volatile(
		"LD %0, R24"
		: "=r" (ti)
	);

	return ti;
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_THREAD_INFO_H */
