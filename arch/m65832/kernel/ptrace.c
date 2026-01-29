// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Ptrace support for the M65832 architecture.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/regset.h>
#include <linux/elf.h>
#include <linux/audit.h>
#include <linux/seccomp.h>

#include <asm/ptrace.h>
#include <asm/syscall.h>

/*
 * Get general purpose registers
 */
static int gpr_get(struct task_struct *target,
		   const struct user_regset *regset,
		   struct membuf to)
{
	return membuf_write(&to, task_pt_regs(target), sizeof(struct pt_regs));
}

/*
 * Set general purpose registers
 */
static int gpr_set(struct task_struct *target,
		   const struct user_regset *regset,
		   unsigned int pos, unsigned int count,
		   const void *kbuf, const void __user *ubuf)
{
	struct pt_regs *regs = task_pt_regs(target);
	int ret;

	ret = user_regset_copyin(&pos, &count, &kbuf, &ubuf, regs, 0, -1);
	if (ret)
		return ret;

	/* Ensure user mode is set */
	regs->status = (regs->status & ~SR_SUPERVISOR) | SR_USER_MODE;

	return 0;
}

/*
 * Register sets
 */
enum m65832_regset {
	REGSET_GPR,
};

static const struct user_regset m65832_regsets[] = {
	[REGSET_GPR] = {
		.core_note_type = NT_PRSTATUS,
		.n = ELF_NGREG,
		.size = sizeof(elf_greg_t),
		.align = sizeof(elf_greg_t),
		.regset_get = gpr_get,
		.set = gpr_set,
	},
};

static const struct user_regset_view m65832_regset_view = {
	.name = "m65832",
	.e_machine = EM_M65832,
	.regsets = m65832_regsets,
	.n = ARRAY_SIZE(m65832_regsets),
};

const struct user_regset_view *task_user_regset_view(struct task_struct *task)
{
	return &m65832_regset_view;
}

/*
 * Read a register by number
 */
long arch_ptrace(struct task_struct *child, long request,
		 unsigned long addr, unsigned long data)
{
	int ret;

	switch (request) {
	default:
		ret = ptrace_request(child, request, addr, data);
		break;
	}

	return ret;
}

/*
 * System call entry tracing
 */
asmlinkage long syscall_trace_enter(struct pt_regs *regs)
{
	long ret = 0;

	if (test_thread_flag(TIF_SYSCALL_TRACE)) {
		if (ptrace_report_syscall_entry(regs))
			return -1;
	}

	/* Seccomp */
	ret = secure_computing();
	if (ret)
		return ret;

	/* Audit */
	if (unlikely(current->audit_context))
		audit_syscall_entry(regs->r0, regs->r1, regs->r2,
				    regs->r3, regs->r4);

	return ret;
}

/*
 * System call exit tracing
 */
asmlinkage void syscall_trace_exit(struct pt_regs *regs)
{
	if (unlikely(current->audit_context))
		audit_syscall_exit(regs);

	if (test_thread_flag(TIF_SYSCALL_TRACE))
		ptrace_report_syscall_exit(regs, 0);
}
