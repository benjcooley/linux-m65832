/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux - Kernel entry point
 *
 * Loaded by boot ROM at physical PHYS_OFFSET (0x00100000) with MMU OFF.
 *
 * Entry conditions (set by boot ROM):
 *   PC  = PHYS_OFFSET (physical)
 *   S=1 (supervisor), E=0 (native), M=10, X=10 (32-bit)
 *   I=1 (IRQs disabled), MMU off
 *   R0  = boot_params pointer (0x00001000)
 *   SP  = 0x0000FFFF (boot ROM stack)
 *   B   = 0 (flat addressing)
 *
 * IMPORTANT: M65832 MMU uses 64-bit page table entries (8 bytes each):
 *   PGD: 1024 entries x 8 bytes = 8KB
 *   PTE: 1024 entries x 8 bytes = 8KB
 *   L1 index: bits 31:22, L2 index: bits 21:12, offset: bits 11:0
 *
 * Before MMU is enabled, all symbol references must be converted:
 *   physical_addr = virtual_addr - LOAD_OFFSET
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>
#include <asm/ptrace.h>

#define LOAD_OFFSET	(PAGE_OFFSET - PHYS_OFFSET)

/*
 * Convert virtual address in A to physical address.
 */
.macro VIRT_TO_PHYS
	SEC
	SBC #LOAD_OFFSET
.endm

	.section ".head.text", "ax"

ENTRY(_start)
	/*
	 * Save boot info pointer from R0.
	 */
	LDA R0
	STA R8
	LDA #boot_info_ptr
	VIRT_TO_PHYS
	STA R9
	LDA R8
	STA (R9)

	/*
	 * Set up kernel stack (physical address).
	 */
	LDA #__end_init_stack
	VIRT_TO_PHYS
	TCS

	/*
	 * Clear BSS section (physical addresses).
	 */
	LDA #__bss_start
	VIRT_TO_PHYS
	STA R0
	LDA #__bss_stop
	VIRT_TO_PHYS
	STA R1
1:
	LDA R0
	CMP R1
	BCS 2f
	LDA #0
	STA (R0)
	LDA R0
	CLC
	ADC #4
	STA R0
	BRA 1b
2:

	/*
	 * Copy exception vector table to physical $0000FFD0.
	 * Hardware reads vectors from physical memory (bypasses MMU).
	 * __vectors_start/__vectors_end are virtual, convert to physical.
	 */
	LDA #__vectors_start
	VIRT_TO_PHYS
	STA R0			/* R0 = source (physical) */
	LDA #0x0000FFD0
	STA R1			/* R1 = destination (physical $FFD0) */
	LDA #__vectors_end
	VIRT_TO_PHYS
	STA R2			/* R2 = source end */
1:
	LDA R0
	CMP R2
	BCS 2f
	LDA (R0)		/* Read vector entry */
	STA (R1)		/* Write to physical $FFD0+ */
	LDA R0
	CLC
	ADC #4
	STA R0
	LDA R1
	CLC
	ADC #4
	STA R1
	BRA 1b
2:

	/*
	 * Set up initial page tables (64-bit entries).
	 * Must use physical address for call (MMU still off).
	 */
	LDA #setup_initial_pgtables
	VIRT_TO_PHYS
	STA R8
	JSR (R8)

	/*
	 * Enable MMU.
	 * System registers at 0xFFFFF0xx bypass MMU.
	 */
	LDA #0xFFFFF000
	STA R8
	LDA #1
	STA (R8)

	/*
	 * MMU is now on. Identity mapping allows continued execution.
	 * Jump to virtual address of start_kernel_virtual.
	 */
	LDA #start_kernel_virtual
	STA R8
	JMP (R8)

start_kernel_virtual:
	/*
	 * Running at virtual addresses. Set up virtual stack.
	 */
	LDA #__end_init_stack
	TCS

	/*
	 * Initialize CPU state for C code:
	 * - Enable register window (RSET): R0-R63 mapped to DP addresses
	 * - Set Direct Page (D) to 0: register window at DP base 0
	 * - B = 0: C function prologues will set B as frame pointer
	 */
	RSET			/* Enable register window */
	LDA #0
	TCD			/* D = 0 (Direct Page base) */
	SB #0			/* B = 0 */

	/*
	 * Early UART output to confirm we reached virtual mode.
	 * Peripheral identity map makes UART accessible at 0x10006010 (TX).
	 * Write 'K' '\r' '\n' to show kernel entry.
	 */
	LDA #0x10006010		/* UART TX register (identity mapped) */
	STA R8
	LDA #0x4B		/* 'K' */
	STA (R8)
	LDA #0x0D		/* '\r' */
	STA (R8)
	LDA #0x0A		/* '\n' */
	STA (R8)

	/*
	 * Call start_kernel (C code).
	 */
	JSR start_kernel

halt_loop:
	WAI
	BRA halt_loop

ENDPROC(_start)

/*
 * setup_initial_pgtables
 *
 * Build early page tables with 64-bit entries (matching MMU hardware).
 * All addresses are PHYSICAL (pre-MMU).
 *
 * PGD: 1024 entries x 8 bytes = 8KB
 * PTE: 1024 entries x 8 bytes = 8KB (x2 tables)
 *
 * Two PTE tables are needed because identity and kernel mappings
 * require different physical base addresses:
 *
 *   PGD[0]   -> identity_pte: maps virtual 0x0 -> physical 0x0
 *               (so code keeps running at physical address after MMU on)
 *
 *   PGD[512] -> kernel_pte: maps virtual 0x80000000 -> physical PHYS_OFFSET
 *               (kernel link address mapping)
 *
 * Subroutine: fill_pte_table
 *   R0 = PTE table base (physical)
 *   R2 = starting physical page | flags
 *   Fills 1024 entries, incrementing by 0x1000 each
 */

ENTRY(setup_initial_pgtables)
	/*
	 * Clear PGD (8192 bytes)
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	STA R0
	LDA R0
	CLC
	ADC #8192
	STA R1
1:
	LDA R0
	CMP R1
	BCS 2f
	LDA #0
	STA (R0)
	LDA R0
	CLC
	ADC #4
	STA R0
	BRA 1b
2:

	/*
	 * PGD[0] = identity_pte | Present | Writable
	 * Identity map: virtual 0x0-0x3FFFFF -> physical 0x0-0x3FFFFF
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	STA R9
	LDA #identity_pte
	VIRT_TO_PHYS
	ORA #3
	STA (R9)		/* Low 32 bits */
	LDA R9
	CLC
	ADC #4
	STA R9
	LDA #0
	STA (R9)		/* High 32 bits = 0 */

	/*
	 * PGD[1] = identity_pte1 | Present | Writable
	 * Identity map: virtual 0x00400000-0x007FFFFF -> physical 0x00400000
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	CLC
	ADC #(1 * 8)
	STA R9
	LDA #identity_pte1
	VIRT_TO_PHYS
	ORA #3
	STA (R9)
	LDA R9
	CLC
	ADC #4
	STA R9
	LDA #0
	STA (R9)

	/*
	 * Kernel mapping: PGD[512..527] for 64MB
	 * Maps virtual 0x80000000-0x83FFFFFF -> physical PHYS_OFFSET
	 * 16 PGD entries x 4MB each = 64MB
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	CLC
	ADC #(512 * 8)		/* &PGD[512] */
	STA R6			/* R6 = current PGD entry address */
	LDA #kernel_pte_area
	VIRT_TO_PHYS
	STA R7			/* R7 = current PTE table address */
	LDA #16
	STA R8			/* R8 = loop counter */
3:
	/* Set PGD entry: PTE table address | Present | Writable */
	LDA R7
	ORA #3
	STA (R6)		/* PGD low 32 bits */
	LDA R6
	CLC
	ADC #4
	STA R9
	LDA #0
	STA (R9)		/* PGD high 32 bits = 0 */
	/* Advance PGD pointer */
	LDA R6
	CLC
	ADC #8
	STA R6
	/* Advance PTE table pointer */
	LDA R7
	CLC
	ADC #8192
	STA R7
	/* Loop */
	DEC R8
	LDA R8
	BNE 3b

	/*
	 * PGD[64] = periph_pte | Present | Writable
	 * Peripheral identity map: virtual 0x10000000-0x103FFFFF -> physical 0x10000000
	 * This allows early UART access at 0x10006000 before ioremap works.
	 * PGD index for 0x10000000 = 0x10000000 >> 22 = 64
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	CLC
	ADC #(64 * 8)
	STA R9
	LDA #periph_pte
	VIRT_TO_PHYS
	ORA #3
	STA (R9)
	LDA R9
	CLC
	ADC #4
	STA R9
	LDA #0
	STA (R9)

	/*
	 * Fill identity_pte: 1024 entries starting at physical 0.
	 * PTE[i] = (i * 0x1000) | Present | Writable
	 */
	LDA #identity_pte
	VIRT_TO_PHYS
	STA R0
	LDA #3			/* Physical page 0 | P | W */
	STA R2
	LDA #1024
	STA R3
	LDA #fill_pte_table
	VIRT_TO_PHYS
	STA R5
	JSR (R5)

	/*
	 * Fill identity_pte1: 1024 entries starting at physical 0x00400000.
	 */
	LDA #identity_pte1
	VIRT_TO_PHYS
	STA R0
	LDA #0x00400000
	ORA #3
	STA R2
	LDA #1024
	STA R3
	LDA #fill_pte_table
	VIRT_TO_PHYS
	STA R5
	JSR (R5)

	/*
	 * Fill all 16 kernel PTE tables: maps 64MB at PHYS_OFFSET.
	 * PTE[i] = (PHYS_OFFSET + table*4MB + i * 0x1000) | Present | Writable
	 */
	LDA #fill_pte_table
	VIRT_TO_PHYS
	STA R5			/* R5 = fill_pte_table (physical) */

	LDA #kernel_pte_area
	VIRT_TO_PHYS
	STA R6			/* R6 = current PTE table */
	LDA #PHYS_OFFSET
	STA R7			/* R7 = current physical base */
	LDA #16
	STA R8			/* R8 = loop counter */
4:
	LDA R6
	STA R0			/* R0 = PTE table base */
	LDA R7
	ORA #3
	STA R2			/* R2 = phys base | flags */
	LDA #1024
	STA R3			/* R3 = entry count */
	JSR (R5)		/* fill_pte_table */
	/* Advance to next PTE table */
	LDA R6
	CLC
	ADC #8192
	STA R6
	/* Advance physical base by 4MB */
	LDA R7
	CLC
	ADC #0x400000
	STA R7
	/* Loop */
	DEC R8
	LDA R8
	BNE 4b

	/*
	 * Fill periph_pte: 1024 entries mapping peripherals at 0x10000000.
	 * PTE[i] = (0x10000000 + i * 0x1000) | Present | Writable
	 */
	LDA #periph_pte
	VIRT_TO_PHYS
	STA R0
	LDA #0x10000000
	ORA #3
	STA R2
	LDA #1024
	STA R3
	JSR (R5)

	/*
	 * Set PTBR.
	 */
	LDA #0xFFFFF014
	STA R8
	LDA #init_pg_dir
	VIRT_TO_PHYS
	STA (R8)		/* PTBR_LO */

	LDA #0xFFFFF018
	STA R8
	LDA #0
	STA (R8)		/* PTBR_HI */

	RTS
ENDPROC(setup_initial_pgtables)

/*
 * fill_pte_table: fill 1024 x 8-byte PTE entries.
 *   R0 = PTE table base (physical address)
 *   R2 = starting physical page address | flags
 *   R3 = count (1024)
 * Clobbers: R0, R2, R3, R4, A
 */
fill_pte_table:
1:
	LDA R2
	STA (R0)		/* PTE low 32 bits */
	LDA R0
	CLC
	ADC #4
	STA R4
	LDA #0
	STA (R4)		/* PTE high 32 bits = 0 */

	LDA R0
	CLC
	ADC #8
	STA R0			/* Next PTE (+8 bytes) */

	LDA R2
	CLC
	ADC #0x1000
	STA R2			/* Next physical page */

	DEC R3
	LDA R3
	BNE 1b

	RTS

/*
 * Data
 */
	.section ".data", "aw"
	.align 4
boot_info_ptr:
	.long	0

/*
 * BSS
 */
	.section ".bss", "aw"
	.align 13			/* 8KB aligned for 64-bit PGD */
	.globl init_pg_dir
init_pg_dir:
	.space	8192			/* 1024 x 8-byte PGD entries */

identity_pte:
	.space	8192			/* 1024 x 8-byte PTE: identity map 0-4MB */

identity_pte1:
	.space	8192			/* 1024 x 8-byte PTE: identity map 4-8MB */

kernel_pte_area:
	.space	8192 * 16		/* 16 x 1024 PTE tables: kernel map 64MB */

periph_pte:
	.space	8192			/* 1024 x 8-byte PTE: peripheral identity map */

	/* init_stack is provided by INIT_TASK_DATA in the linker script.
	 * __start_init_stack = init_stack = init_thread_union
	 * __end_init_stack = init_stack + THREAD_SIZE
	 * SP should be set to __end_init_stack (stack grows down). */
