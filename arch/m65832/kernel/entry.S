/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Low-level entry points for exceptions, interrupts, and system calls.
 *
 * M65832 register transfer mnemonics:
 *   TSC = SP -> A,  TCS = A -> SP
 *   TAX = A -> X,   TXA = X -> A
 *   TAY = A -> Y,   TYA = Y -> A
 *   TAB = A -> B (frame pointer), TBA = B -> A
 *   PHP = push status, PLA = pull A from stack
 *
 * Stack-relative addressing: LDA offset,S / STA offset,S
 *   8-bit offset from current SP (0-255).
 */

#include <linux/linkage.h>
#include <linux/errno.h>
#include <asm/ptrace.h>
#include <asm/unistd.h>

/*
 * Save all registers to the pt_regs area on the stack.
 * SP has already been adjusted to point to the base of pt_regs.
 * Uses stack-relative addressing (offset,S) for all accesses.
 */
.macro SAVE_ALL
	/* Save A first (we need it as scratch) */
	STA PT_A,S

	/* Save B register (frame pointer) via A */
	TBA				/* B -> A */
	STA PT_B,S

	/* Save X and Y (no STX/STY sr,S mode - go through A) */
	TXA				/* X -> A */
	STA PT_X,S
	TYA				/* Y -> A */
	STA PT_Y,S

	/* Save R0-R7 (argument registers) */
	LDA R0
	STA PT_R0,S
	LDA R1
	STA PT_R1,S
	LDA R2
	STA PT_R2,S
	LDA R3
	STA PT_R3,S
	LDA R4
	STA PT_R4,S
	LDA R5
	STA PT_R5,S
	LDA R6
	STA PT_R6,S
	LDA R7
	STA PT_R7,S

	/* Save R8-R15 (temporaries) */
	LDA R8
	STA PT_R8,S
	LDA R9
	STA PT_R9,S
	LDA R10
	STA PT_R10,S
	LDA R11
	STA PT_R11,S
	LDA R12
	STA PT_R12,S
	LDA R13
	STA PT_R13,S
	LDA R14
	STA PT_R14,S
	LDA R15
	STA PT_R15,S

	/* Save R16-R23 (callee-saved) */
	LDA R16
	STA PT_R16,S
	LDA R17
	STA PT_R17,S
	LDA R18
	STA PT_R18,S
	LDA R19
	STA PT_R19,S
	LDA R20
	STA PT_R20,S
	LDA R21
	STA PT_R21,S
	LDA R22
	STA PT_R22,S
	LDA R23
	STA PT_R23,S

	/* Save R26-R30 (R30 = link register) */
	LDA R26
	STA PT_R26,S
	LDA R27
	STA PT_R27,S
	LDA R28
	STA PT_R28,S
	LDA R29
	STA PT_R29,S
	LDA R30
	STA PT_R30,S
.endm

/*
 * Restore all registers from the pt_regs area on the stack.
 */
.macro RESTORE_ALL
	/* Restore R0-R7 */
	LDA PT_R0,S
	STA R0
	LDA PT_R1,S
	STA R1
	LDA PT_R2,S
	STA R2
	LDA PT_R3,S
	STA R3
	LDA PT_R4,S
	STA R4
	LDA PT_R5,S
	STA R5
	LDA PT_R6,S
	STA R6
	LDA PT_R7,S
	STA R7

	/* Restore R8-R15 */
	LDA PT_R8,S
	STA R8
	LDA PT_R9,S
	STA R9
	LDA PT_R10,S
	STA R10
	LDA PT_R11,S
	STA R11
	LDA PT_R12,S
	STA R12
	LDA PT_R13,S
	STA R13
	LDA PT_R14,S
	STA R14
	LDA PT_R15,S
	STA R15

	/* Restore R16-R23 */
	LDA PT_R16,S
	STA R16
	LDA PT_R17,S
	STA R17
	LDA PT_R18,S
	STA R18
	LDA PT_R19,S
	STA R19
	LDA PT_R20,S
	STA R20
	LDA PT_R21,S
	STA R21
	LDA PT_R22,S
	STA R22
	LDA PT_R23,S
	STA R23

	/* Restore R26-R30 */
	LDA PT_R26,S
	STA R26
	LDA PT_R27,S
	STA R27
	LDA PT_R28,S
	STA R28
	LDA PT_R29,S
	STA R29
	LDA PT_R30,S
	STA R30

	/* Restore B (frame pointer) */
	LDA PT_B,S
	TAB				/* A -> B */

	/* Restore X and Y (no LDX/LDY sr,S mode - go through A) */
	LDA PT_X,S
	TAX				/* A -> X */
	LDA PT_Y,S
	TAY				/* A -> Y */

	/* Restore A last */
	LDA PT_A,S
.endm

/*
 * Hardware exception frame offsets (above pt_regs area).
 * After SP is adjusted down by PT_SIZE, the hardware-saved
 * status and PC are at these offsets from new SP.
 */
.set _EXC_STATUS_OFF, PT_SIZE
.set _EXC_PC_OFF, PT_SIZE + 4

/*
 * Exception entry - common code for all exceptions
 */
.macro EXCEPTION_ENTRY vector_name
	/* Adjust SP to make room for pt_regs */
	TSC				/* SP -> A */
	SEC
	SBC #PT_SIZE			/* Subtract pt_regs size */
	TCS				/* A -> SP */

	/* Save all registers */
	SAVE_ALL

	/* Save exception PC and status from hardware frame */
	LDA _EXC_STATUS_OFF,S		/* Get saved status */
	STA PT_STATUS,S
	LDA _EXC_PC_OFF,S		/* Get saved PC */
	STA PT_PC,S
.endm

/*
 * Exception return
 */
.macro EXCEPTION_RETURN
	/* Check if returning to user mode */
	LDA PT_STATUS,S
	AND #SR_SUPERVISOR
	BNE 1f

	/* Returning to user mode - check for pending work */
	/* TODO: Check TIF flags via m65832_current_task */

1:
	/* Restore all registers */
	RESTORE_ALL

	/* Adjust stack (remove pt_regs) and return */
	TSC				/* SP -> A */
	CLC
	ADC #PT_SIZE
	TCS				/* A -> SP */

	RTI
.endm

/*
 * System call entry point (TRAP/SYSCALL vector)
 */
	.section ".text", "ax"
	.align 4
ENTRY(system_call)
	EXCEPTION_ENTRY syscall

	/* Save original R0 for restart */
	LDA PT_R0,S
	STA PT_ORIG_R0,S

	/* R0 contains syscall number, R1-R7 contain arguments */

	/* Validate syscall number */
	LDA R0
	CMP #NR_syscalls
	BCS invalid_syscall

	/* Get syscall table entry: R8 = &sys_call_table[syscall_nr] */
	ASL A				/* Multiply by 4 */
	ASL A
	CLC
	ADC #sys_call_table
	STA R8

	/* Call the syscall handler */
	LDA (R8)			/* Load handler address */
	STA R9
	JSR (R9)			/* Call handler indirectly */

	/* Store return value */
	STA PT_R0,S

syscall_return:
	EXCEPTION_RETURN

invalid_syscall:
	LDA #-ENOSYS
	STA PT_R0,S
	BRA syscall_return
ENDPROC(system_call)

/*
 * IRQ entry point
 */
	.align 4
ENTRY(irq_entry)
	EXCEPTION_ENTRY irq

	/* Call C IRQ handler with pt_regs pointer in R0 */
	TSC				/* SP -> A (= pt_regs base) */
	STA R0				/* R0 = pt_regs pointer */
	JSR do_IRQ

	EXCEPTION_RETURN
ENDPROC(irq_entry)

/*
 * Page fault entry point
 */
	.align 4
ENTRY(page_fault_entry)
	EXCEPTION_ENTRY page_fault

	/* Read fault address from FAULTVA register */
	LDA $FFFFF010			/* FAULTVA */
	STA R1				/* R1 = fault address */

	/* Pass pt_regs pointer as first argument */
	TSC				/* SP -> A */
	STA R0				/* R0 = pt_regs pointer */

	JSR do_page_fault

	EXCEPTION_RETURN
ENDPROC(page_fault_entry)

/*
 * Illegal instruction entry point
 */
	.align 4
ENTRY(illegal_insn_entry)
	EXCEPTION_ENTRY illegal_insn

	TSC
	STA R0
	JSR do_illegal_insn

	EXCEPTION_RETURN
ENDPROC(illegal_insn_entry)

/*
 * NMI entry point
 */
	.align 4
ENTRY(nmi_entry)
	EXCEPTION_ENTRY nmi

	TSC
	STA R0
	JSR do_nmi

	EXCEPTION_RETURN
ENDPROC(nmi_entry)

/*
 * Return from fork - child returns here
 */
	.align 4
ENTRY(ret_from_fork)
	EXCEPTION_RETURN
ENDPROC(ret_from_fork)

/*
 * Return from kernel thread
 */
	.align 4
ENTRY(ret_from_kernel_thread)
	/* R1 = function pointer, R0 = argument (set by copy_thread) */
	LDA R0
	JSR (R1)
	JSR do_exit
ENDPROC(ret_from_kernel_thread)

/*
 * Exception vector table
 */
	.section ".vectors", "ax"
	.align 4
exception_vectors:
	.long	page_fault_entry	/* Page Fault */
	.long	0			/* Reserved */
	.long	system_call		/* SYSCALL */
	.long	0			/* Reserved */
	.long	0			/* COP */
	.long	0			/* COP */
	.long	0			/* BRK */
	.long	0			/* ABORT */
	.long	nmi_entry		/* NMI */
	.long	0			/* Reserved */
	.long	irq_entry		/* IRQ */
	.long	0			/* Reserved */
	.long	illegal_insn_entry	/* Illegal Instruction */
	.long	0			/* Reset */
