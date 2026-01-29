// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Process management for the M65832 architecture.
 */

#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/sched/task.h>
#include <linux/sched/task_stack.h>
#include <linux/tick.h>
#include <linux/ptrace.h>
#include <linux/kallsyms.h>
#include <linux/pm.h>
#include <linux/reboot.h>

#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/switch_to.h>
#include <asm/current.h>

/*
 * Power off function pointer
 */
void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

/*
 * Idle loop - wait for interrupts
 */
void arch_cpu_idle(void)
{
	raw_local_irq_enable();
	/* WAI instruction - wait for interrupt */
	asm volatile("wai" : : : "memory");
}

/*
 * Halt the system
 */
void machine_halt(void)
{
	local_irq_disable();
	while (1)
		asm volatile("wai");
}

/*
 * Power off the system
 */
void machine_power_off(void)
{
	if (pm_power_off)
		pm_power_off();
	machine_halt();
}

/*
 * Restart the system
 */
void machine_restart(char *cmd)
{
	local_irq_disable();
	/* TODO: Implement proper reset via system register */
	/* For now, just halt */
	machine_halt();
}

/*
 * Show register state for debugging
 */
void show_regs(struct pt_regs *regs)
{
	pr_info("CPU: 0  PID: %d  Comm: %s\n",
		current->pid, current->comm);
	pr_info("PC: %08lx  Status: %08lx\n", regs->pc, regs->status);
	pr_info(" R0: %08lx  R1: %08lx  R2: %08lx  R3: %08lx\n",
		regs->r0, regs->r1, regs->r2, regs->r3);
	pr_info(" R4: %08lx  R5: %08lx  R6: %08lx  R7: %08lx\n",
		regs->r4, regs->r5, regs->r6, regs->r7);
	pr_info(" R8: %08lx  R9: %08lx R10: %08lx R11: %08lx\n",
		regs->r8, regs->r9, regs->r10, regs->r11);
	pr_info("R12: %08lx R13: %08lx R14: %08lx R15: %08lx\n",
		regs->r12, regs->r13, regs->r14, regs->r15);
	pr_info("R29: %08lx (FP) R30: %08lx (LR)\n", regs->r29, regs->r30);
	pr_info("  A: %08lx   X: %08lx   Y: %08lx  SP: %08lx\n",
		regs->a, regs->x, regs->y, regs->sp);
}

/*
 * Get the program counter for a waiting process
 */
unsigned long __get_wchan(struct task_struct *p)
{
	unsigned long pc;

	if (!p || p == current || task_is_running(p))
		return 0;

	/* Get saved PC from kernel stack */
	pc = task_pt_regs(p)->pc;
	return pc;
}

/*
 * Copy thread state for fork/clone
 */
int copy_thread(struct task_struct *p, const struct kernel_clone_args *args)
{
	unsigned long clone_flags = args->flags;
	unsigned long usp = args->stack;
	unsigned long tls = args->tls;
	struct pt_regs *childregs;
	struct thread_struct *thread = &p->thread;

	/* Get pointer to child's pt_regs at top of kernel stack */
	childregs = task_pt_regs(p);

	/* Clear the register frame */
	memset(childregs, 0, sizeof(*childregs));

	if (unlikely(args->fn)) {
		/* Kernel thread */
		childregs->r0 = (unsigned long)args->fn_arg;
		childregs->r1 = (unsigned long)args->fn;
		childregs->pc = (unsigned long)ret_from_kernel_thread;
		childregs->status = SR_KERNEL_MODE;
		p->thread.ksp = (unsigned long)childregs;
		return 0;
	}

	/* User thread - copy parent's registers */
	*childregs = *current_pt_regs();

	/* Child returns 0 from fork */
	childregs->r0 = 0;

	/* Use provided stack or parent's stack */
	if (usp)
		childregs->sp = usp;

	/* Set up TLS if requested */
	if (clone_flags & CLONE_SETTLS)
		/* TODO: Set TLS pointer - arch specific */
		;

	/* Save kernel stack pointer for context switch */
	p->thread.ksp = (unsigned long)childregs;

	return 0;
}

/*
 * Start a new thread executing in user mode
 */
void start_thread(struct pt_regs *regs, unsigned long pc, unsigned long sp)
{
	memset(regs, 0, sizeof(*regs));
	regs->pc = pc;
	regs->sp = sp;
	regs->status = SR_USER_MODE;
}

/*
 * Release all thread resources
 */
void release_thread(struct task_struct *dead_task)
{
	/* Nothing to do */
}

/*
 * Exit the thread
 */
void exit_thread(struct task_struct *tsk)
{
	/* Nothing to do */
}

/*
 * Flush thread state (exec)
 */
void flush_thread(void)
{
#ifdef CONFIG_M65832_FPU
	/* Clear FPU state on exec */
	memset(&current->thread.fpu_state, 0, sizeof(current->thread.fpu_state));
#endif
}

/*
 * Assembly helpers - defined in entry.S
 */
asmlinkage void ret_from_fork(void);
asmlinkage void ret_from_kernel_thread(void);

/*
 * Low-level context switch
 * Saves callee-saved registers of 'prev' and restores 'next'
 */
struct task_struct *__switch_to(struct task_struct *prev,
				struct task_struct *next)
{
	struct thread_struct *prev_thread = &prev->thread;
	struct thread_struct *next_thread = &next->thread;

	/*
	 * Save prev's callee-saved registers
	 * These are saved on the kernel stack in entry.S
	 */

	/*
	 * Update thread_info and task pointers in reserved registers
	 * R24 = thread_info
	 * R25 = task_struct
	 */
	asm volatile(
		"LD R25, %0\n\t"	/* current = next */
		"LD R24, %1"		/* thread_info = next's thread_info */
		:
		: "r" (next), "r" (task_thread_info(next))
		: "memory"
	);

	/* Switch kernel stack pointer */
	asm volatile(
		"LD %0, SP\n\t"		/* Save prev's ksp */
		"LD SP, %1"		/* Load next's ksp */
		: "=r" (prev_thread->ksp)
		: "r" (next_thread->ksp)
		: "memory"
	);

	return prev;
}
