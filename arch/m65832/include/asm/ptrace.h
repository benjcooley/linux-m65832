/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Register and ptrace definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_PTRACE_H
#define _ASM_M65832_PTRACE_H

#include <uapi/asm/ptrace.h>

#ifndef __ASSEMBLY__

/*
 * Status register bits
 */
#define SR_CARRY	(1 << 0)	/* C - Carry flag */
#define SR_ZERO		(1 << 1)	/* Z - Zero flag */
#define SR_IRQ_DISABLE	(1 << 2)	/* I - IRQ disable */
#define SR_DECIMAL	(1 << 3)	/* D - Decimal mode (disabled in Linux) */
#define SR_INDEX_8	(1 << 4)	/* X0 - Index register width bit 0 */
#define SR_ACCUM_8	(1 << 5)	/* M0 - Accumulator width bit 0 */
#define SR_OVERFLOW	(1 << 6)	/* V - Overflow flag */
#define SR_NEGATIVE	(1 << 7)	/* N - Negative flag */
#define SR_EMULATION	(1 << 8)	/* E - Emulation mode (must be 0) */
#define SR_INDEX_16	(1 << 9)	/* X1 - Index register width bit 1 */
#define SR_ACCUM_16	(1 << 10)	/* M1 - Accumulator width bit 1 */
#define SR_REGWIN	(1 << 11)	/* R - Register window mode */
#define SR_SUPERVISOR	(1 << 12)	/* S - Supervisor mode */
#define SR_MMU_ENABLE	(1 << 13)	/* MM - MMU enabled */

/* User mode status register value (32-bit mode, user, MMU on) */
#define SR_USER_MODE	(SR_MMU_ENABLE | SR_ACCUM_16 | SR_ACCUM_8 | SR_INDEX_16 | SR_INDEX_8)

/* Kernel mode status register value */
#define SR_KERNEL_MODE	(SR_SUPERVISOR | SR_MMU_ENABLE | SR_ACCUM_16 | SR_ACCUM_8 | SR_INDEX_16 | SR_INDEX_8)

/*
 * This struct defines the way the registers are stored on the stack during
 * a system call or exception.
 */
struct pt_regs {
	/* General purpose registers (register window R0-R31) */
	unsigned long r0, r1, r2, r3, r4, r5, r6, r7;		/* Arguments/return */
	unsigned long r8, r9, r10, r11, r12, r13, r14, r15;	/* Temp/caller-saved */
	unsigned long r16, r17, r18, r19, r20, r21, r22, r23;	/* Callee-saved */
	unsigned long r24;	/* thread_info pointer (reserved) */
	unsigned long r25;	/* current task pointer (reserved) */
	unsigned long r26, r27, r28;	/* Reserved */
	unsigned long r29;	/* Frame pointer */
	unsigned long r30;	/* Return address / link register */

	/* Legacy 6502 registers (aliased but saved separately) */
	unsigned long a;	/* Accumulator */
	unsigned long x;	/* X index */
	unsigned long y;	/* Y index */

	/* Control registers */
	unsigned long sp;	/* Stack pointer */
	unsigned long pc;	/* Program counter */
	unsigned long status;	/* Status register (P) */

	/* Exception info */
	unsigned long orig_r0;	/* Original R0 for syscall restart */
};

#define user_mode(regs)		(!((regs)->status & SR_SUPERVISOR))
#define kernel_mode(regs)	((regs)->status & SR_SUPERVISOR)

#define instruction_pointer(regs)	((regs)->pc)
#define user_stack_pointer(regs)	((regs)->sp)
#define profile_pc(regs)		instruction_pointer(regs)

static inline long regs_return_value(struct pt_regs *regs)
{
	return regs->r0;
}

static inline void instruction_pointer_set(struct pt_regs *regs,
					   unsigned long val)
{
	regs->pc = val;
}

/* Query interrupt status */
#define interrupts_enabled(regs)	(!((regs)->status & SR_IRQ_DISABLE))

/*
 * Offsets for stack frame (in bytes)
 */
#define PT_R0		0
#define PT_R1		4
#define PT_R2		8
#define PT_R3		12
#define PT_R4		16
#define PT_R5		20
#define PT_R6		24
#define PT_R7		28
#define PT_R8		32
#define PT_R9		36
#define PT_R10		40
#define PT_R11		44
#define PT_R12		48
#define PT_R13		52
#define PT_R14		56
#define PT_R15		60
#define PT_R16		64
#define PT_R17		68
#define PT_R18		72
#define PT_R19		76
#define PT_R20		80
#define PT_R21		84
#define PT_R22		88
#define PT_R23		92
#define PT_R24		96
#define PT_R25		100
#define PT_R26		104
#define PT_R27		108
#define PT_R28		112
#define PT_R29		116
#define PT_R30		120
#define PT_A		124
#define PT_X		128
#define PT_Y		132
#define PT_SP		136
#define PT_PC		140
#define PT_STATUS	144
#define PT_ORIG_R0	148
#define PT_SIZE		152

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_PTRACE_H */
