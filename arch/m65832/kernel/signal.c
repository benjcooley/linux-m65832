// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Signal handling for the M65832 architecture.
 */

#include <linux/sched.h>
#include <linux/sched/task_stack.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/resume_user_mode.h>

#include <asm/uaccess.h>
#include <asm/ptrace.h>
#include <asm/ucontext.h>
#include <asm/sigframe.h>

/*
 * Signal frame structure on user stack
 */
struct sigframe {
	struct sigcontext sc;
	unsigned long extramask[_NSIG_WORDS - 1];
	unsigned long retcode[2];	/* sigreturn trampoline */
};

struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
	unsigned long retcode[2];
};

/*
 * Save register state to sigcontext
 */
static int setup_sigcontext(struct sigcontext __user *sc, struct pt_regs *regs,
			    unsigned long mask)
{
	int err = 0;

	err |= __put_user(regs->r0, &sc->r0);
	err |= __put_user(regs->r1, &sc->r1);
	err |= __put_user(regs->r2, &sc->r2);
	err |= __put_user(regs->r3, &sc->r3);
	err |= __put_user(regs->r4, &sc->r4);
	err |= __put_user(regs->r5, &sc->r5);
	err |= __put_user(regs->r6, &sc->r6);
	err |= __put_user(regs->r7, &sc->r7);
	err |= __put_user(regs->r29, &sc->r29);
	err |= __put_user(regs->r30, &sc->r30);
	err |= __put_user(regs->a, &sc->a);
	err |= __put_user(regs->x, &sc->x);
	err |= __put_user(regs->y, &sc->y);
	err |= __put_user(regs->sp, &sc->sp);
	err |= __put_user(regs->pc, &sc->pc);
	err |= __put_user(regs->status, &sc->status);
	err |= __put_user(mask, &sc->oldmask);

	return err;
}

/*
 * Restore register state from sigcontext
 */
static int restore_sigcontext(struct pt_regs *regs, struct sigcontext __user *sc)
{
	int err = 0;

	err |= __get_user(regs->r0, &sc->r0);
	err |= __get_user(regs->r1, &sc->r1);
	err |= __get_user(regs->r2, &sc->r2);
	err |= __get_user(regs->r3, &sc->r3);
	err |= __get_user(regs->r4, &sc->r4);
	err |= __get_user(regs->r5, &sc->r5);
	err |= __get_user(regs->r6, &sc->r6);
	err |= __get_user(regs->r7, &sc->r7);
	err |= __get_user(regs->r29, &sc->r29);
	err |= __get_user(regs->r30, &sc->r30);
	err |= __get_user(regs->a, &sc->a);
	err |= __get_user(regs->x, &sc->x);
	err |= __get_user(regs->y, &sc->y);
	err |= __get_user(regs->sp, &sc->sp);
	err |= __get_user(regs->pc, &sc->pc);
	err |= __get_user(regs->status, &sc->status);

	/* Ensure user mode */
	regs->status = (regs->status & ~SR_SUPERVISOR) | SR_USER_MODE;

	return err;
}

/*
 * Get user stack pointer for signal frame
 */
static inline void __user *get_sigframe(struct ksignal *ksig,
					struct pt_regs *regs, size_t frame_size)
{
	unsigned long sp = regs->sp;

	/* Align stack */
	sp = (sp - frame_size) & ~3UL;

	return (void __user *)sp;
}

/*
 * Set up a signal frame
 */
static int setup_frame(struct ksignal *ksig, sigset_t *set,
		       struct pt_regs *regs)
{
	struct sigframe __user *frame;
	int err = 0;

	frame = get_sigframe(ksig, regs, sizeof(*frame));
	if (!access_ok(frame, sizeof(*frame)))
		return -EFAULT;

	err |= setup_sigcontext(&frame->sc, regs, set->sig[0]);
	if (_NSIG_WORDS > 1)
		err |= __copy_to_user(frame->extramask, &set->sig[1],
				      sizeof(frame->extramask));

	/* Set up to return from signal handler */
	/* Trampoline: call sigreturn syscall */
	err |= __put_user(0xA9000000 | __NR_sigreturn, &frame->retcode[0]); /* LDA #syscall */
	err |= __put_user(0x00000000, &frame->retcode[1]); /* TRAP */

	if (err)
		return -EFAULT;

	/* Set up registers for signal handler */
	regs->sp = (unsigned long)frame;
	regs->pc = (unsigned long)ksig->ka.sa.sa_handler;
	regs->r0 = ksig->sig;				/* Signal number */
	regs->r30 = (unsigned long)&frame->retcode;	/* Return address */

	return 0;
}

/*
 * Set up a RT signal frame
 */
static int setup_rt_frame(struct ksignal *ksig, sigset_t *set,
			  struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	int err = 0;

	frame = get_sigframe(ksig, regs, sizeof(*frame));
	if (!access_ok(frame, sizeof(*frame)))
		return -EFAULT;

	err |= copy_siginfo_to_user(&frame->info, &ksig->info);

	/* Set up ucontext */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(NULL, &frame->uc.uc_link);
	err |= __save_altstack(&frame->uc.uc_stack, regs->sp);
	err |= setup_sigcontext(&frame->uc.uc_mcontext, regs, 0);
	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));

	/* Set up return trampoline */
	err |= __put_user(0xA9000000 | __NR_rt_sigreturn, &frame->retcode[0]);
	err |= __put_user(0x00000000, &frame->retcode[1]);

	if (err)
		return -EFAULT;

	/* Set up registers for signal handler */
	regs->sp = (unsigned long)frame;
	regs->pc = (unsigned long)ksig->ka.sa.sa_handler;
	regs->r0 = ksig->sig;				/* Signal number */
	regs->r1 = (unsigned long)&frame->info;		/* siginfo */
	regs->r2 = (unsigned long)&frame->uc;		/* ucontext */
	regs->r30 = (unsigned long)&frame->retcode;	/* Return address */

	return 0;
}

/*
 * Handle signal delivery
 */
static void handle_signal(struct ksignal *ksig, struct pt_regs *regs)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;

	if (ksig->ka.sa.sa_flags & SA_SIGINFO)
		ret = setup_rt_frame(ksig, oldset, regs);
	else
		ret = setup_frame(ksig, oldset, regs);

	signal_setup_done(ret, ksig, 0);
}

/*
 * Main signal handling entry point
 * Called from entry.S before returning to userspace
 */
asmlinkage void do_signal(struct pt_regs *regs)
{
	struct ksignal ksig;

	if (get_signal(&ksig)) {
		handle_signal(&ksig, regs);
		return;
	}

	/* No signal to deliver - restore saved mask if needed */
	restore_saved_sigmask();
}

/*
 * sigreturn system call
 */
asmlinkage long sys_sigreturn(struct pt_regs *regs)
{
	struct sigframe __user *frame = (struct sigframe __user *)regs->sp;
	sigset_t set;

	if (!access_ok(frame, sizeof(*frame)))
		goto badframe;

	if (__get_user(set.sig[0], &frame->sc.oldmask))
		goto badframe;

	if (_NSIG_WORDS > 1) {
		if (__copy_from_user(&set.sig[1], frame->extramask,
				     sizeof(frame->extramask)))
			goto badframe;
	}

	set_current_blocked(&set);

	if (restore_sigcontext(regs, &frame->sc))
		goto badframe;

	return regs->r0;

badframe:
	force_sig(SIGSEGV);
	return 0;
}

/*
 * rt_sigreturn system call
 */
asmlinkage long sys_rt_sigreturn(struct pt_regs *regs)
{
	struct rt_sigframe __user *frame = (struct rt_sigframe __user *)regs->sp;
	sigset_t set;

	if (!access_ok(frame, sizeof(*frame)))
		goto badframe;

	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(regs, &frame->uc.uc_mcontext))
		goto badframe;

	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return regs->r0;

badframe:
	force_sig(SIGSEGV);
	return 0;
}
