/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Register and ptrace definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_PTRACE_H
#define _ASM_M65832_PTRACE_H

#include <uapi/asm/ptrace.h>

/*
 * Status register bits (needed by both C and assembly).
 * Use literal hex values so the assembler can parse them directly.
 *
 * M65832 P register layout (14 bits):
 *   Bits 0-7:  C, Z, I, D, X, M, V, N  (65816-identical low byte)
 *   Bits 8-9:  W0, W1                   (width mode, thermometer encoding)
 *   Bit  10:   reserved                 (future W2 for 64-bit)
 *   Bit  11:   S                        (supervisor mode)
 *   Bit  12:   R                        (register window)
 *   Bit  13:   K                        (compatibility mode)
 *
 * Width mode (W1:W0):  00=emulation, 01=native-16, 11=32-bit
 * E flag is derived: E = (W==00)
 */
#define SR_CARRY	0x0001		/* C - Carry flag */
#define SR_ZERO		0x0002		/* Z - Zero flag */
#define SR_IRQ_DISABLE	0x0004		/* I - IRQ disable */
#define SR_DECIMAL	0x0008		/* D - Decimal mode (disabled in Linux) */
#define SR_INDEX	0x0010		/* X - Index width (65816 compat) */
#define SR_ACCUM	0x0020		/* M - Accumulator width (65816 compat) */
#define SR_OVERFLOW	0x0040		/* V - Overflow flag */
#define SR_NEGATIVE	0x0080		/* N - Negative flag */
#define SR_W0		0x0100		/* W0 - Width mode bit 0 */
#define SR_W1		0x0200		/* W1 - Width mode bit 1 */
#define SR_W_MASK	0x0300		/* W1:W0 mask */
#define SR_SUPERVISOR	0x0800		/* S - Supervisor mode */
#define SR_REGWIN	0x1000		/* R - Register window mode */
#define SR_COMPAT	0x2000		/* K - Compatibility mode */

/* Width mode values (W1:W0) */
#define SR_W_EMU	0x0000		/* W=00: 6502 emulation */
#define SR_W_16		0x0100		/* W=01: 65816 native-16 */
#define SR_W_32		0x0300		/* W=11: 32-bit native */

/* User mode: 32-bit, register window */
#define SR_USER_MODE	(SR_W_32 | SR_REGWIN)			/* 0x1300 */

/* Kernel mode: 32-bit, supervisor, register window */
#define SR_KERNEL_MODE	(SR_W_32 | SR_SUPERVISOR | SR_REGWIN)	/* 0x1B00 */

/*
 * Offsets into pt_regs (in bytes) - needed by both C and assembly
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
#define PT_B		128
#define PT_X		132
#define PT_Y		136
#define PT_SP		140
#define PT_PC		144
#define PT_STATUS	148
#define PT_ORIG_R0	152
#define PT_SIZE		156

/* ---- C-only definitions below ---- */
#ifndef __ASSEMBLY__

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
	unsigned long r26, r27, r28, r29;	/* Reserved (kernel) */
	unsigned long r30;	/* Link register (lr) - return address */

	/* 6502-style registers */
	unsigned long a;	/* A accumulator */
	unsigned long b;	/* B register - frame pointer (fp) */
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

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_PTRACE_H */
