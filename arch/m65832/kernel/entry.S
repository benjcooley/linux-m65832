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
 * HACK(m65832-boot): include/generated/asm-offsets.h currently emits invalid
 * numeric macro names for pt_regs fields, which breaks assembly inclusion.
 * Keep the minimal switch offsets local until asm-offsets generation is fixed.
 */
.set _TASK_THREAD_OFF, 1264
.set _THREAD_KSP_OFF, 0
.set _THREAD_R16_OFF, 4
.set _THREAD_R17_OFF, 8
.set _THREAD_R18_OFF, 12
.set _THREAD_R19_OFF, 16
.set _THREAD_R20_OFF, 20
.set _THREAD_R21_OFF, 24
.set _THREAD_R22_OFF, 28
.set _THREAD_R23_OFF, 32
.set _THREAD_R24_OFF, 36
.set _THREAD_R25_OFF, 40
.set _THREAD_B_OFF, 44
.set _THREAD_START_PC_OFF, 48
.set _THREAD_START_ARG0_OFF, 52
.set _THREAD_START_ARG1_OFF, 56
.set _THREAD_STARTED_OFF, 60

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

	/* Save R24-R30 (R30 = link register) */
	LDA R24
	STA PT_R24,S
	LDA R25
	STA PT_R25,S
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

	/* Restore R24-R30 */
	LDA PT_R24,S
	STA R24
	LDA PT_R25,S
	STA R25
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
 *
 * push8() writes to memory[SP] then SP--.  The 6-byte hardware
 * exception frame (P_low, P_high, PC[7:0]..PC[31:24]) sits at
 * hw_SP+1 through hw_SP+6 after the hardware push.
 *
 * On entry we PHA to save original A (4 bytes in 32-bit W=11 mode)
 * before clobbering it for the SP adjustment.  Stack layout after
 * PHA + SBC #PT_SIZE + TCS, from new_SP upward:
 *
 *   [new_SP + 0 .. new_SP + PT_SIZE-1]        pt_regs area
 *   [new_SP + PT_SIZE + 1 .. +PT_SIZE + 4]    PHA'd original A (4 bytes)
 *   [new_SP + PT_SIZE + 5 .. +PT_SIZE + 6]    hardware P (2 bytes)
 *   [new_SP + PT_SIZE + 7 .. +PT_SIZE + 10]   hardware PC (4 bytes)
 */
.set _EXC_SAVED_A_OFF, PT_SIZE + 1
.set _EXC_STATUS_OFF, PT_SIZE + 5
.set _EXC_PC_OFF, PT_SIZE + 7

/*
 * Exception entry - common code for all exceptions
 */
.macro EXCEPTION_ENTRY vector_name
	/* Save original A before clobbering it (PHA pushes 4 bytes in W=11) */
	PHA

	/* Adjust SP to make room for pt_regs */
	TSC				/* SP -> A */
	SEC
	SBC #PT_SIZE			/* Subtract pt_regs size */
	TCS				/* A -> SP */

	/* Save all registers */
	SAVE_ALL

	/* Save original SP: new_SP + PT_SIZE + PHA(4) + hw_frame(6) */
	TSC				/* current SP -> A */
	CLC
	ADC #(PT_SIZE + 10)		/* original SP before exception */
	STA PT_SP,S

	/* Save exception PC and status from hardware frame */
	LDA _EXC_PC_OFF,S		/* Get saved PC (32-bit) */
	STA PT_PC,S
	LDA _EXC_STATUS_OFF,S		/* Get saved status (32-bit load) */
	AND #$FFFF			/* Mask to 16-bit status only */
	STA PT_STATUS,S
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
	/*
	 * Do not rewrite hardware status/PC slots here.
	 *
	 * M65832 exception-frame slot width/layout is still under validation;
	 * writing 32-bit values into stack-relative slots has been observed to
	 * corrupt return state on interrupt exit in early boot.
	 *
	 * RTI will consume the original hardware-pushed frame.
	 */

	/* Restore all registers */
	RESTORE_ALL

	/* Adjust stack: remove pt_regs area (PHA slot now at SP+1) */
	TSC				/* SP -> A */
	CLC
	ADC #PT_SIZE
	TCS				/* A -> SP */

	/* Restore original A from PHA slot */
	PLA

	/* Hardware frame is now at SP+1..SP+6; RTI pulls P and PC */
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
 * Low-level task context switch.
 * Arguments:
 *   R0 = prev task_struct *
 *   R1 = next task_struct *
 * Returns:
 *   R0 = prev task_struct *
 */
	.align 4
ENTRY(__switch_to)
	/* Keep prev pointer for return value */
	LDA R0
	STA R7

	/* prev_thread = prev + TASK_THREAD */
	CLC
	ADC #_TASK_THREAD_OFF
	STA R2

	/* Save prev kernel SP */
	TSC
	LDY #_THREAD_KSP_OFF
	STA (R2),Y

	/* Save callee-saved registers */
	LDA R16
	LDY #_THREAD_R16_OFF
	STA (R2),Y
	LDA R17
	LDY #_THREAD_R17_OFF
	STA (R2),Y
	LDA R18
	LDY #_THREAD_R18_OFF
	STA (R2),Y
	LDA R19
	LDY #_THREAD_R19_OFF
	STA (R2),Y
	LDA R20
	LDY #_THREAD_R20_OFF
	STA (R2),Y
	LDA R21
	LDY #_THREAD_R21_OFF
	STA (R2),Y
	LDA R22
	LDY #_THREAD_R22_OFF
	STA (R2),Y
	LDA R23
	LDY #_THREAD_R23_OFF
	STA (R2),Y
	LDA R24
	LDY #_THREAD_R24_OFF
	STA (R2),Y
	LDA R25
	LDY #_THREAD_R25_OFF
	STA (R2),Y

	/* Save frame pointer B */
	TBA
	LDY #_THREAD_B_OFF
	STA (R2),Y

	/* next_thread = next + TASK_THREAD */
	LDA R1
	CLC
	ADC #_TASK_THREAD_OFF
	STA R3

	/* Switch to next kernel SP */
	LDY #_THREAD_KSP_OFF
	LDA (R3),Y
	TCS

	/* Restore callee-saved registers */
	LDY #_THREAD_R16_OFF
	LDA (R3),Y
	STA R16
	LDY #_THREAD_R17_OFF
	LDA (R3),Y
	STA R17
	LDY #_THREAD_R18_OFF
	LDA (R3),Y
	STA R18
	LDY #_THREAD_R19_OFF
	LDA (R3),Y
	STA R19
	LDY #_THREAD_R20_OFF
	LDA (R3),Y
	STA R20
	LDY #_THREAD_R21_OFF
	LDA (R3),Y
	STA R21
	LDY #_THREAD_R22_OFF
	LDA (R3),Y
	STA R22
	LDY #_THREAD_R23_OFF
	LDA (R3),Y
	STA R23
	LDY #_THREAD_R24_OFF
	LDA (R3),Y
	STA R24
	LDY #_THREAD_R25_OFF
	LDA (R3),Y
	STA R25

	/* Restore frame pointer B */
	LDY #_THREAD_B_OFF
	LDA (R3),Y
	TAB

	/* Update current task pointer */
	LDA #m65832_current_task
	STA R4
	LDA R1
	LDY #0
	STA (R4),Y

	/* First activation of a freshly forked task: jump to entry trampoline. */
	LDY #_THREAD_STARTED_OFF
	LDA (R3),Y
	CMP #0
	BNE 1f
	LDA #1
	STA (R3),Y
	/* Match generic fork entry contract for new tasks. */
	LDA R7
	STA R0
	JSR schedule_tail
	LDA #m65832_current_task
	STA R4
	LDY #0
	LDA (R4),Y
	CLC
	ADC #_TASK_THREAD_OFF
	STA R3
	LDY #_THREAD_START_ARG0_OFF
	LDA (R3),Y
	STA R0
	LDY #_THREAD_START_ARG1_OFF
	LDA (R3),Y
	STA R1
	LDY #_THREAD_START_PC_OFF
	LDA (R3),Y
	STA R5
	JMP (R5)

	/* Return prev task pointer */
1:
	LDA R7
	STA R0
	RTS
ENDPROC(__switch_to)

/*
 * Exception vector table - M65832 Extended Vectors
 *
 * Must be placed at PHYSICAL addresses $0000FFD0-$0000FFFF.
 * Vector reads bypass MMU (hardware reads from physical memory).
 * Each entry is 4 bytes (32-bit handler address).
 *
 * Hardware vector layout:
 *   $FFD0: Page Fault
 *   $FFD4: SYSCALL (TRAP base)
 *   $FFD8: Reserved
 *   $FFDC: Reserved
 *   $FFE0: Reserved
 *   $FFE4: COP
 *   $FFE6: BRK      (note: 2-byte entries in legacy region)
 *   $FFE8: ABORT
 *   $FFEA: NMI
 *   $FFEC: Reserved
 *   $FFEE: IRQ
 *   $FFF0: Reserved
 *   $FFF4: Reserved
 *   $FFF8: Illegal Instruction
 *   $FFFC: Reset
 */
	.section ".vectors", "ax"

	/*
	 * M65832 exception vector table.
	 * Placed at physical $0000FFD0 by the linker script.
	 *
	 * HACK(m65832-irq): hardware fetches IRQ vector as a 32-bit value
	 * starting at $FFEE, so place irq_entry at +$1E explicitly.
	 */
	.org	0x00
	.long	page_fault_entry	/* +$00 = $FFD0: Page Fault */
	.org	0x04
	.long	system_call		/* +$04 = $FFD4: SYSCALL */
	.org	0x08
	.long	0			/* +$08 = $FFD8: Reserved */
	.org	0x0C
	.long	0			/* +$0C = $FFDC: Reserved */
	.org	0x10
	.long	0			/* +$10 = $FFE0: Reserved */
	.org	0x14
	.long	0			/* +$14 = $FFE4: COP */
	.org	0x18
	.long	0			/* +$18 = $FFE8: ABORT */
	.org	0x1C
	.short	0			/* +$1C = $FFEC: NMI placeholder */
	.org	0x1E
	.long	irq_entry		/* +$1E = $FFEE: IRQ */
	.org	0x24
	.long	0			/* +$24 = $FFF4: Reserved */
	.org	0x28
	.long	illegal_insn_entry	/* +$28 = $FFF8: Illegal Instruction */
	.org	0x2C
	.long	0			/* +$2C = $FFFC: Reset */
