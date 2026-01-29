// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Page fault handler.
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>
#include <linux/perf_event.h>

#include <asm/ptrace.h>
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

	/*
	 * If we're in interrupt context or have no user context,
	 * we can't handle the fault gracefully.
	 */
	if (faulthandler_disabled() || !mm) {
		goto no_context;
	}

	if (user_mode(regs))
		flags |= FAULT_FLAG_USER;

	/*
	 * Determine if this was a read or write fault
	 * TODO: Read fault type from hardware register
	 */
	/* For now, assume write fault if in kernel mode writing */

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

	if (expand_stack(vma, address))
		goto bad_area;

good_area:
	/*
	 * Check permissions
	 */
	/* TODO: Check read/write/execute permissions based on fault type */

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
	return;

bad_area:
	mmap_read_unlock(mm);

	if (user_mode(regs)) {
		/* Send SIGSEGV to user process */
		force_sig_fault(SIGSEGV, SEGV_MAPERR, (void __user *)address);
		return;
	}

no_context:
	/*
	 * Kernel mode page fault with no handler
	 */
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
