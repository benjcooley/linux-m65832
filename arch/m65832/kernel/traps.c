// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Exception and trap handling.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/debug.h>
#include <linux/signal.h>
#include <linux/kdebug.h>
#include <linux/module.h>

#include <asm/ptrace.h>
#include <asm/traps.h>
#include <asm/setup.h>

/* show_regs is defined in process.c */
extern void show_regs(struct pt_regs *regs);

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
	/* nmi_enter/nmi_exit replaced by irqentry_nmi_enter/exit in modern kernels */
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

/* show_regs is defined in process.c */

/*
 * Check if an address is a valid BUG() location
 */
int is_valid_bugaddr(unsigned long addr)
{
	return 1;
}
