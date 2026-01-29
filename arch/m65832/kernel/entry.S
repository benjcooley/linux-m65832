/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Low-level entry points for exceptions, interrupts, and system calls.
 *
 * Exception vectors (native mode):
 *   0x0000FFD0 - Page Fault
 *   0x0000FFD4 - SYSCALL (TRAP instruction)
 *   0x0000FFE4 - COP
 *   0x0000FFE6 - BRK
 *   0x0000FFE8 - ABORT
 *   0x0000FFEA - NMI
 *   0x0000FFEE - IRQ
 *   0x0000FFF8 - Illegal Instruction
 */

#include <linux/linkage.h>
#include <asm/ptrace.h>

/*
 * Save all registers to the stack
 * Expects SP to point to top of pt_regs area
 */
.macro SAVE_ALL
	/* Save general registers R0-R30 */
	sta	PT_R0(sp)
	stx	PT_X(sp)
	sty	PT_Y(sp)

	/* Save legacy A register separately */
	pha
	pla
	sta	PT_A(sp)

	/* Save R0-R30 from register window */
	/* TODO: Use register window if enabled */
	lda	r0
	sta	PT_R0(sp)
	lda	r1
	sta	PT_R1(sp)
	lda	r2
	sta	PT_R2(sp)
	lda	r3
	sta	PT_R3(sp)
	lda	r4
	sta	PT_R4(sp)
	lda	r5
	sta	PT_R5(sp)
	lda	r6
	sta	PT_R6(sp)
	lda	r7
	sta	PT_R7(sp)
	/* ... continue for r8-r30 ... */
.endm

/*
 * Restore all registers from the stack
 */
.macro RESTORE_ALL
	/* Restore general registers */
	lda	PT_R0(sp)
	sta	r0
	lda	PT_R1(sp)
	sta	r1
	/* ... continue for other registers ... */

	/* Restore legacy registers */
	lda	PT_A(sp)
	ldx	PT_X(sp)
	ldy	PT_Y(sp)
.endm

/*
 * Exception entry - common code for all exceptions
 */
.macro EXCEPTION_ENTRY vector_name
	/* Adjust SP to make room for pt_regs */
	tsa			/* Transfer SP to A */
	sec
	sbc	#PT_SIZE	/* Subtract pt_regs size */
	tas			/* Transfer A to SP */

	/* Save all registers */
	SAVE_ALL

	/* Save exception PC and status (pushed by hardware) */
	/* PC and P were pushed by the exception */
	lda	PT_SIZE(sp)	/* Get saved P */
	sta	PT_STATUS(sp)
	lda	PT_SIZE+2(sp)	/* Get saved PC (32-bit) */
	sta	PT_PC(sp)
.endm

/*
 * Exception return
 */
.macro EXCEPTION_RETURN
	/* Check if returning to user mode */
	lda	PT_STATUS(sp)
	and	#SR_SUPERVISOR
	bne	1f

	/* Returning to user mode - check for pending work */
	lda	r24		/* thread_info pointer */
	lda	(r24)		/* flags */
	and	#_TIF_WORK_MASK
	beq	1f

	/* Handle pending work */
	jsr	do_work_pending

1:
	/* Restore all registers */
	RESTORE_ALL

	/* Adjust stack and return */
	tsa
	clc
	adc	#PT_SIZE
	tas

	rti
.endm

/*
 * System call entry point (TRAP/SYSCALL vector at 0x0000FFD4)
 */
	.section ".text", "ax"
	.align 4
ENTRY(system_call)
	EXCEPTION_ENTRY syscall

	/* Save original R0 for restart */
	lda	PT_R0(sp)
	sta	PT_ORIG_R0(sp)

	/* R0 contains syscall number */
	/* R1-R7 contain arguments */

	/* Validate syscall number */
	lda	r0
	cmp	#NR_syscalls
	bcs	invalid_syscall

	/* Get syscall table entry */
	asl	a		/* Multiply by 4 (pointer size) */
	asl	a
	clc
	adc	#sys_call_table
	sta	r8		/* r8 = &sys_call_table[syscall_nr] */

	/* Call the syscall handler */
	lda	(r8)		/* Load handler address */
	jsr	(a)		/* Call handler */

	/* Store return value */
	sta	PT_R0(sp)

syscall_return:
	EXCEPTION_RETURN

invalid_syscall:
	lda	#-ENOSYS
	sta	PT_R0(sp)
	bra	syscall_return
ENDPROC(system_call)

/*
 * IRQ entry point (vector at 0x0000FFEE)
 */
	.align 4
ENTRY(irq_entry)
	EXCEPTION_ENTRY irq

	/* Call C IRQ handler */
	lda	sp		/* Pass pt_regs pointer */
	jsr	do_IRQ

	EXCEPTION_RETURN
ENDPROC(irq_entry)

/*
 * Page fault entry point (vector at 0x0000FFD0)
 */
	.align 4
ENTRY(page_fault_entry)
	EXCEPTION_ENTRY page_fault

	/* Read fault address from FAULTVA register */
	lda	$FFFFF010	/* FAULTVA */
	sta	r1		/* Pass as second argument */

	/* Pass pt_regs pointer as first argument */
	lda	sp
	sta	r0

	/* Call C page fault handler */
	jsr	do_page_fault

	EXCEPTION_RETURN
ENDPROC(page_fault_entry)

/*
 * Illegal instruction entry point (vector at 0x0000FFF8)
 */
	.align 4
ENTRY(illegal_insn_entry)
	EXCEPTION_ENTRY illegal_insn

	lda	sp		/* Pass pt_regs pointer */
	jsr	do_illegal_insn

	EXCEPTION_RETURN
ENDPROC(illegal_insn_entry)

/*
 * NMI entry point (vector at 0x0000FFEA)
 */
	.align 4
ENTRY(nmi_entry)
	EXCEPTION_ENTRY nmi

	lda	sp
	jsr	do_nmi

	EXCEPTION_RETURN
ENDPROC(nmi_entry)

/*
 * Handle pending work before returning to userspace
 */
ENTRY(do_work_pending)
	/* Check for signals */
	lda	r24
	lda	(r24)
	and	#_TIF_SIGPENDING
	beq	1f
	jsr	do_signal

1:
	/* Check for reschedule */
	lda	r24
	lda	(r24)
	and	#_TIF_NEED_RESCHED
	beq	2f
	jsr	schedule

2:
	rts
ENDPROC(do_work_pending)

/*
 * Exception vector table (placed at 0xFFFF_FF00 or similar)
 * These are just the vector addresses - actual handlers are above
 */
	.section ".vectors", "ax"
	.align 4
exception_vectors:
	.long	page_fault_entry	/* 0xFFD0 - Page Fault */
	.long	0			/* 0xFFD4 - Reserved */
	.long	system_call		/* 0xFFD8 - SYSCALL */
	.long	0			/* 0xFFDC - Reserved */
	.long	0			/* 0xFFE0 - COP (unused) */
	.long	0			/* 0xFFE4 - COP */
	.long	0			/* 0xFFE8 - BRK */
	.long	0			/* 0xFFEC - ABORT */
	.long	nmi_entry		/* 0xFFEA - NMI */
	.long	0			/* 0xFFEC - Reserved */
	.long	irq_entry		/* 0xFFEE - IRQ */
	.long	0			/* 0xFFF0-FFF4 - Reserved */
	.long	illegal_insn_entry	/* 0xFFF8 - Illegal Instruction */
	.long	0			/* 0xFFFC - Reset (not used in Linux) */
