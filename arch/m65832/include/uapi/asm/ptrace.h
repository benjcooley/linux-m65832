/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * M65832 Linux
 *
 * User-space ptrace register definitions.
 */

#ifndef _UAPI_ASM_M65832_PTRACE_H
#define _UAPI_ASM_M65832_PTRACE_H

/*
 * Register numbers for ptrace
 */
#define M65832_REG_R0		0
#define M65832_REG_R1		1
#define M65832_REG_R2		2
#define M65832_REG_R3		3
#define M65832_REG_R4		4
#define M65832_REG_R5		5
#define M65832_REG_R6		6
#define M65832_REG_R7		7
#define M65832_REG_R8		8
#define M65832_REG_R9		9
#define M65832_REG_R10		10
#define M65832_REG_R11		11
#define M65832_REG_R12		12
#define M65832_REG_R13		13
#define M65832_REG_R14		14
#define M65832_REG_R15		15
#define M65832_REG_R16		16
#define M65832_REG_R17		17
#define M65832_REG_R18		18
#define M65832_REG_R19		19
#define M65832_REG_R20		20
#define M65832_REG_R21		21
#define M65832_REG_R22		22
#define M65832_REG_R23		23
#define M65832_REG_R24		24
#define M65832_REG_R25		25
#define M65832_REG_R26		26
#define M65832_REG_R27		27
#define M65832_REG_R28		28
#define M65832_REG_R29		29
#define M65832_REG_R30		30
#define M65832_REG_A		31
#define M65832_REG_X		32
#define M65832_REG_Y		33
#define M65832_REG_SP		34
#define M65832_REG_PC		35
#define M65832_REG_STATUS	36
#define M65832_NUM_REGS		37

/*
 * User-space view of registers
 */
struct user_regs_struct {
	unsigned long regs[31];		/* R0-R30 */
	unsigned long a, x, y;		/* Legacy registers */
	unsigned long sp;		/* Stack pointer */
	unsigned long pc;		/* Program counter */
	unsigned long status;		/* Status register */
};

#endif /* _UAPI_ASM_M65832_PTRACE_H */
