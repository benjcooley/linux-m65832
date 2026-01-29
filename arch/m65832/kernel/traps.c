// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Exception and trap handling.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>
#include <linux/kdebug.h>

#include <asm/ptrace.h>
#include <asm/traps.h>
#include <asm/setup.h>

/*
 * Generic exception handler
 */
static void do_trap(struct pt_regs *regs, unsigned long code, const char *str)
{
	if (user_mode(regs)) {
		/* User mode - send signal */
		force_sig(SIGILL);
		return;
	}

	/* Kernel mode - oops */
	pr_emerg("Kernel exception: %s\n", str);
	pr_emerg("PC: %08lx  Status: %08lx\n", regs->pc, regs->status);
	pr_emerg("R0: %08lx R1: %08lx R2: %08lx R3: %08lx\n",
		 regs->r0, regs->r1, regs->r2, regs->r3);
	pr_emerg("R4: %08lx R5: %08lx R6: %08lx R7: %08lx\n",
		 regs->r4, regs->r5, regs->r6, regs->r7);
	pr_emerg("SP: %08lx\n", regs->sp);

	die("Oops", regs, code);
}

/*
 * Illegal instruction handler
 */
asmlinkage void do_illegal_insn(struct pt_regs *regs)
{
	do_trap(regs, 0, "Illegal instruction");
}

/*
 * NMI handler
 */
asmlinkage void do_nmi(struct pt_regs *regs)
{
	pr_emerg("NMI received!\n");
	pr_emerg("PC: %08lx  Status: %08lx\n", regs->pc, regs->status);

	/* NMI is typically fatal or for debugging */
	nmi_enter();
	/* Handle NMI - could be watchdog, hardware error, etc. */
	nmi_exit();
}

/*
 * Initialize exception vectors
 */
void __init trap_init(void)
{
	early_printk("M65832: Setting up exception handlers\n");

	/*
	 * The M65832 uses fixed exception vector addresses.
	 * The vector table is in entry.S and linked at the
	 * appropriate addresses.
	 *
	 * Nothing to do here for now - vectors are set up statically.
	 */
}

/*
 * Die - kernel panic from exception
 */
void die(const char *str, struct pt_regs *regs, long err)
{
	static int die_counter;
	int ret;

	oops_enter();

	spin_lock_irq(&die_lock);
	console_verbose();

	pr_emerg("Oops: %s [#%d]\n", str, ++die_counter);
	print_modules();

	show_regs(regs);

	ret = notify_die(DIE_OOPS, str, regs, err, 0, SIGSEGV);

	spin_unlock_irq(&die_lock);
	oops_exit();

	if (in_interrupt())
		panic("Fatal exception in interrupt");
	if (panic_on_oops)
		panic("Fatal exception");

	make_task_dead(SIGSEGV);
}
EXPORT_SYMBOL(die);

/*
 * Show registers for debugging
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
	pr_info("R16: %08lx R17: %08lx R18: %08lx R19: %08lx\n",
		regs->r16, regs->r17, regs->r18, regs->r19);
	pr_info("R20: %08lx R21: %08lx R22: %08lx R23: %08lx\n",
		regs->r20, regs->r21, regs->r22, regs->r23);
	pr_info("R29: %08lx (FP) R30: %08lx (LR)\n", regs->r29, regs->r30);
	pr_info("  A: %08lx   X: %08lx   Y: %08lx  SP: %08lx\n",
		regs->a, regs->x, regs->y, regs->sp);
}
