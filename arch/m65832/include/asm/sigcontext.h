/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Signal context structure.
 */

#ifndef _ASM_M65832_SIGCONTEXT_H
#define _ASM_M65832_SIGCONTEXT_H

/*
 * Signal context structure - saved on user stack during signal delivery
 */
struct sigcontext {
	/* General purpose registers */
	unsigned long r0, r1, r2, r3, r4, r5, r6, r7;
	unsigned long r8, r9, r10, r11, r12, r13, r14, r15;
	unsigned long r16, r17, r18, r19, r20, r21, r22, r23;
	unsigned long r29;	/* Frame pointer */
	unsigned long r30;	/* Link register */

	/* Legacy 6502 registers */
	unsigned long a, x, y;

	/* Control registers */
	unsigned long sp;
	unsigned long pc;
	unsigned long status;

	/* Signal mask */
	unsigned long oldmask;

	/* FPU state (if FPU enabled) */
	unsigned long fpregs[32];	/* F0-F15 as 64-bit */
	unsigned long fpsr;
};

#endif /* _ASM_M65832_SIGCONTEXT_H */
