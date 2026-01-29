/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Kernel entry point - this is where the bootloader transfers control.
 *
 * Entry conditions:
 *   - Running in supervisor mode (S=1)
 *   - MMU disabled (MM=0)
 *   - 32-bit native mode (E=0, M=11, X=11)
 *   - Physical memory accessible
 *   - R0 = pointer to boot info structure (optional)
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/ptrace.h>
#include <asm/page.h>

	.section ".head.text", "ax"

/*
 * Kernel entry point
 */
ENTRY(_start)
	/*
	 * Ensure we're in 32-bit native mode with supervisor privileges
	 * Set: E=0, M=11 (32-bit A), X=11 (32-bit X/Y), S=1, I=1
	 */
	sep	#SR_IRQ_DISABLE		/* Disable IRQs */
	clc				/* Clear carry for native mode */

	/*
	 * Save boot info pointer (passed in R0)
	 */
	sta	boot_info_ptr

	/*
	 * Set up initial stack pointer
	 * Use a fixed location in the .bss section for early boot
	 */
	lda	#init_stack_top
	tas				/* Transfer A to Stack Pointer */

	/*
	 * Clear BSS section
	 */
	lda	#__bss_start
	sta	r0			/* Start address */
	lda	#__bss_stop
	sta	r1			/* End address */
	lda	#0
1:
	cmp	r0, r1			/* Compare current with end */
	bcs	2f			/* Branch if done */
	sta	(r0)			/* Store zero */
	inc	r0			/* Increment by 4 (word size) */
	inc	r0
	inc	r0
	inc	r0
	bra	1b
2:

	/*
	 * Set up initial page tables
	 * For early boot, create a simple identity mapping for the kernel
	 */
	jsr	setup_initial_pgtables

	/*
	 * Enable MMU
	 */
	lda	#1
	sta	$FFFFF000		/* MMUCR - enable MMU */

	/*
	 * Now running with MMU enabled
	 * Jump to the kernel's virtual address
	 */
	lda	#start_kernel_virtual
	jmp	(a)

start_kernel_virtual:
	/*
	 * Set up thread_info pointer in R24
	 */
	lda	#init_thread_info
	sta	r24

	/*
	 * Set up current task pointer in R25
	 */
	lda	#init_task
	sta	r25

	/*
	 * Call the C startup code
	 */
	jsr	start_kernel

	/*
	 * If start_kernel returns (it shouldn't), halt
	 */
halt_loop:
	wai				/* Wait for interrupt */
	bra	halt_loop

ENDPROC(_start)

/*
 * Set up initial page tables for early boot
 * Creates identity mapping for low memory and kernel mapping
 */
ENTRY(setup_initial_pgtables)
	/*
	 * M65832 uses 2-level page tables:
	 * - L1 (PGD): 1024 entries, each covering 4MB
	 * - L2 (PTE): 1024 entries, each covering 4KB
	 *
	 * For early boot:
	 * - Identity map physical 0x00000000-0x00400000 (first 4MB)
	 * - Map virtual 0x80000000+ to physical 0x00000000+
	 */

	/* Clear the initial page directory */
	lda	#init_pg_dir
	sta	r0
	lda	#(init_pg_dir + 4096)
	sta	r1
	lda	#0
1:
	cmp	r0, r1
	bcs	2f
	sta	(r0)
	inc	r0
	inc	r0
	inc	r0
	inc	r0
	bra	1b
2:

	/* Set up identity mapping for first 4MB (entry 0) */
	lda	#init_pte_table
	ora	#0x003			/* Present + Writable */
	sta	init_pg_dir

	/* Set up kernel mapping (entry 512 for 0x80000000) */
	lda	#init_pte_table
	ora	#0x003			/* Present + Writable */
	sta	init_pg_dir + (512 * 4)

	/* Fill the PTE table for first 4MB (identity and kernel) */
	lda	#init_pte_table
	sta	r0
	lda	#0x003			/* Start at physical 0, Present + Writable */
	sta	r2
	lda	#1024			/* 1024 PTEs */
	sta	r3
3:
	lda	r2
	sta	(r0)
	clc
	lda	r2
	adc	#0x1000			/* Next 4KB page */
	sta	r2
	clc
	lda	r0
	adc	#4			/* Next PTE entry */
	sta	r0
	dec	r3
	bne	3b

	/* Set the page table base register */
	lda	#init_pg_dir
	sta	$FFFFF014		/* PTBR_LO */
	lda	#0
	sta	$FFFFF018		/* PTBR_HI */

	rts
ENDPROC(setup_initial_pgtables)

/*
 * Data section
 */
	.section ".data", "aw"

	.align 4
boot_info_ptr:
	.long	0

/*
 * BSS section
 */
	.section ".bss", "aw"

	.align 12			/* Page aligned */
init_pg_dir:
	.space	4096			/* 1024 PGD entries */

init_pte_table:
	.space	4096			/* 1024 PTE entries */

	.align 12
init_stack:
	.space	8192			/* 8KB initial stack */
init_stack_top:
