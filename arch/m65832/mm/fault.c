// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Page fault handler.
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>
#include <linux/sched/debug.h>
#include <linux/perf_event.h>

#include <asm/ptrace.h>
#include <asm/traps.h>
#include <asm/mmu.h>

/*
 * Page fault handler
 *
 * This is called from entry.S when a page fault exception occurs.
 * The faulting address is read from the FAULTVA register.
 */
asmlinkage void do_page_fault(struct pt_regs *regs, unsigned long address)
{
	struct task_struct *tsk = current;
	struct mm_struct *mm = tsk->mm;

	struct vm_area_struct *vma;
	vm_fault_t fault;
	unsigned int flags = FAULT_FLAG_DEFAULT;
	static int fault_recursion;

	/*
	 * Prevent recursive page faults: if we fault inside the fault
	 * handler (e.g. printk → kallsyms → unmapped page), just halt
	 * instead of spiralling into a stack overflow.
	 */
	if (fault_recursion) {
		/* Recursive fault in fault handler: stop to preserve state. */
		asm volatile(".byte 0xDB");  /* STP */
	}
	fault_recursion++;

	/*
	 * If we're in interrupt context or have no user context,
	 * we can't handle the fault gracefully.
	 */
	if (faulthandler_disabled() || !mm) {
		goto no_context;
	}

	if (user_mode(regs))
		flags |= FAULT_FLAG_USER;

	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, regs, address);

retry:
	mmap_read_lock(mm);

	vma = find_vma(mm, address);
	if (!vma)
		goto bad_area;

	if (vma->vm_start <= address)
		goto good_area;

	if (!(vma->vm_flags & VM_GROWSDOWN))
		goto bad_area;

	vma = expand_stack(mm, address);
	if (!vma)
		goto bad_area_nosemaphore;

good_area:
	/*
	 * Handle the fault
	 */
	fault = handle_mm_fault(vma, address, flags, regs);

	if (fault_signal_pending(fault, regs)) {
		if (!user_mode(regs))
			goto no_context;
		return;
	}

	if (unlikely(fault & VM_FAULT_ERROR)) {
		if (fault & VM_FAULT_OOM)
			goto out_of_memory;
		if (fault & VM_FAULT_SIGBUS)
			goto do_sigbus;
		BUG();
	}

	if (fault & VM_FAULT_RETRY) {
		flags |= FAULT_FLAG_TRIED;
		goto retry;
	}

	mmap_read_unlock(mm);
	fault_recursion--;
	return;

bad_area:
	mmap_read_unlock(mm);
bad_area_nosemaphore:
	if (user_mode(regs)) {
		force_sig_fault(SIGSEGV, SEGV_MAPERR, (void __user *)address);
		fault_recursion--;
		return;
	}

no_context:
	{
		/*
		 * Direct UART for guaranteed output before die() which
		 * may trigger additional page faults.
		 */
		volatile unsigned int *uart_tx = (volatile unsigned int *)0x10006000;
		volatile unsigned int *uart_st = (volatile unsigned int *)0x10006004;
		char buf[80];
		const char *p;
		int len;

		len = snprintf(buf, sizeof(buf),
			       "PAGE FAULT: addr=%08lx PC=%08lx\n",
			       address, regs->pc);
		for (p = buf; *p; p++) {
			while (!((*uart_st) & 0x02))
				;
			*uart_tx = *p;
		}
	}
	pr_emerg("Unable to handle kernel %s at virtual address %08lx\n",
		 address < PAGE_SIZE ? "NULL pointer dereference" : "paging request",
		 address);
	pr_emerg("PC: %08lx\n", regs->pc);
	die("Oops", regs, address);

out_of_memory:
	mmap_read_unlock(mm);
	if (!user_mode(regs))
		goto no_context;
	pagefault_out_of_memory();
	return;

do_sigbus:
	mmap_read_unlock(mm);
	if (!user_mode(regs))
		goto no_context;
	force_sig_fault(SIGBUS, BUS_ADRERR, (void __user *)address);
}
