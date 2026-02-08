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
	LDA #init_stack_top
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
	LDA #init_stack_top
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
	 * PGD[512] = kernel_pte0 | Present | Writable
	 * Kernel map: virtual 0x80000000-0x803FFFFF -> physical PHYS_OFFSET
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	CLC
	ADC #(512 * 8)
	STA R9
	LDA #kernel_pte
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
	 * PGD[513] = kernel_pte1 | Present | Writable
	 * Kernel map: virtual 0x80400000-0x807FFFFF -> physical PHYS_OFFSET+0x400000
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	CLC
	ADC #(513 * 8)
	STA R9
	LDA #kernel_pte1
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
	 * Fill kernel_pte: 1024 entries mapping 4MB at PHYS_OFFSET.
	 * PTE[i] = (PHYS_OFFSET + i * 0x1000) | Present | Writable
	 */
	LDA #kernel_pte
	VIRT_TO_PHYS
	STA R0
	LDA #PHYS_OFFSET
	ORA #3
	STA R2
	LDA #1024
	STA R3
	LDA #fill_pte_table
	VIRT_TO_PHYS
	STA R5
	JSR (R5)

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
	LDA #fill_pte_table
	VIRT_TO_PHYS
	STA R5
	JSR (R5)

	/*
	 * Fill kernel_pte1: 1024 entries mapping next 4MB.
	 * PTE[i] = (PHYS_OFFSET + 0x400000 + i * 0x1000) | Present | Writable
	 */
	LDA #kernel_pte1
	VIRT_TO_PHYS
	STA R0
	LDA #PHYS_OFFSET
	CLC
	ADC #0x400000		/* 4MB offset for second PTE table */
	ORA #3
	STA R2
	LDA #1024
	STA R3
	LDA #fill_pte_table
	VIRT_TO_PHYS
	STA R5
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
init_pg_dir:
	.space	8192			/* 1024 x 8-byte PGD entries */

identity_pte:
	.space	8192			/* 1024 x 8-byte PTE: identity map */

kernel_pte:
	.space	8192			/* 1024 x 8-byte PTE: kernel map 0-4MB */

kernel_pte1:
	.space	8192			/* 1024 x 8-byte PTE: kernel map 4-8MB */

periph_pte:
	.space	8192			/* 1024 x 8-byte PTE: peripheral identity map */

	.align 12
init_stack:
	.space	8192			/* 8KB kernel stack */
init_stack_top:
