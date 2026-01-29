/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * User context structure for signal handling.
 */

#ifndef _ASM_M65832_UCONTEXT_H
#define _ASM_M65832_UCONTEXT_H

#include <asm/sigcontext.h>

struct ucontext {
	unsigned long		uc_flags;
	struct ucontext		*uc_link;
	stack_t			uc_stack;
	struct sigcontext	uc_mcontext;
	sigset_t		uc_sigmask;
};

#endif /* _ASM_M65832_UCONTEXT_H */
