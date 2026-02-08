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
 * Memory layout:
 *   Virtual 0x80000000+ = Physical PHYS_OFFSET+ (kernel)
 *   LOAD_OFFSET = PAGE_OFFSET - PHYS_OFFSET = 0x7FF00000
 *
 * Before MMU is enabled, all symbol references must be converted:
 *   physical_addr = virtual_addr - LOAD_OFFSET
 *
 * Register-indirect addressing is used for all memory access since
 * standard 'abs' mode is B-relative (16-bit offset from B register).
 * LDA #symbol gives a 32-bit immediate (virtual address).
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>
#include <asm/ptrace.h>

/*
 * LOAD_OFFSET: subtract from virtual address to get physical.
 * PAGE_OFFSET = 0x80000000, PHYS_OFFSET = 0x00100000
 * LOAD_OFFSET = 0x7FF00000
 */
#define LOAD_OFFSET	(PAGE_OFFSET - PHYS_OFFSET)

/*
 * Macro: convert virtual address in A to physical address.
 * Used before MMU is enabled.
 */
.macro VIRT_TO_PHYS
	SEC
	SBC #LOAD_OFFSET
.endm

	.section ".head.text", "ax"

ENTRY(_start)
	/*
	 * IRQs already disabled by boot ROM (I=1).
	 * B=0 set by boot ROM (flat addressing for abs mode).
	 *
	 * Save boot info pointer from R0.
	 * boot_info_ptr is a virtual address, convert to physical.
	 */
	LDA R0
	STA R8			/* R8 = boot params pointer */
	LDA #boot_info_ptr
	VIRT_TO_PHYS
	STA R9			/* R9 = &boot_info_ptr (physical) */
	LDA R8
	STA (R9)		/* Store boot params pointer */

	/*
	 * Set up kernel stack (physical address).
	 * init_stack_top is in BSS at a virtual address.
	 */
	LDA #init_stack_top
	VIRT_TO_PHYS
	TCS			/* A -> SP */

	/*
	 * Clear BSS section (physical addresses).
	 */
	LDA #__bss_start
	VIRT_TO_PHYS
	STA R0			/* R0 = physical __bss_start */
	LDA #__bss_stop
	VIRT_TO_PHYS
	STA R1			/* R1 = physical __bss_stop */
1:
	LDA R0
	CMP R1
	BCS 2f			/* Done when R0 >= R1 */
	LDA #0
	STA (R0)		/* Zero one word */
	LDA R0
	CLC
	ADC #4
	STA R0
	BRA 1b
2:

	/*
	 * Set up initial page tables.
	 * Creates:
	 *   - Identity map: phys 0x00000000-0x003FFFFF -> virt 0x00000000-0x003FFFFF
	 *   - Kernel map:   phys 0x00000000-0x003FFFFF -> virt 0x80000000-0x803FFFFF
	 *
	 * Must use physical address for call (MMU still off).
	 * JSR (Rn) uses the address in the register as the call target.
	 */
	LDA #setup_initial_pgtables
	VIRT_TO_PHYS
	STA R8
	JSR (R8)

	/*
	 * Enable MMU: write 1 to MMUCR at 0xFFFFF000.
	 * System registers bypass MMU, so this address works regardless.
	 */
	LDA #0xFFFFF000
	STA R8
	LDA #1
	STA (R8)

	/*
	 * MMU is now on. Identity mapping means we can still execute
	 * at our current physical address. Jump to the virtual address
	 * of start_kernel_virtual (0x80xxxxxx).
	 *
	 * start_kernel_virtual is a virtual address (linker-resolved),
	 * which now works because the kernel mapping is active.
	 */
	LDA #start_kernel_virtual
	STA R8
	JMP (R8)

start_kernel_virtual:
	/*
	 * Now running at virtual addresses with MMU on.
	 * Set up the virtual-address stack pointer.
	 */
	LDA #init_stack_top
	TCS

	/*
	 * Call start_kernel (C code, virtual addresses).
	 * m65832_current_task is set to &init_task in process.c.
	 */
	JSR start_kernel

	/* Should never return */
halt_loop:
	WAI
	BRA halt_loop

ENDPROC(_start)

/*
 * setup_initial_pgtables
 *
 * Build early page tables for identity + kernel mapping.
 * All addresses here are PHYSICAL (pre-MMU).
 *
 * Page table structure (2-level):
 *   PGD: 1024 entries x 4 bytes = 4KB, each entry covers 4MB
 *   PTE: 1024 entries x 4 bytes = 4KB, each entry covers 4KB
 *
 * We create one PTE table mapping physical 0-4MB, and point
 * both PGD[0] (identity) and PGD[512] (kernel at 0x80000000) to it.
 */
ENTRY(setup_initial_pgtables)
	/*
	 * Clear PGD (4096 bytes)
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	STA R0
	LDA R0
	CLC
	ADC #4096
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
	 * PGD[0] = init_pte_table (physical) | Present | Writable
	 * Identity mapping: virtual 0x00000000 -> physical 0x00000000
	 */
	LDA #init_pte_table
	VIRT_TO_PHYS
	ORA #3			/* Present + Writable */
	STA R2			/* R2 = PTE table phys | flags */

	LDA #init_pg_dir
	VIRT_TO_PHYS
	STA R8			/* R8 = &PGD[0] (physical) */
	LDA R2
	STA (R8)

	/*
	 * PGD[512] = same PTE table | Present | Writable
	 * Kernel mapping: virtual 0x80000000 -> physical 0x00000000
	 */
	LDA #init_pg_dir
	VIRT_TO_PHYS
	CLC
	ADC #(512 * 4)
	STA R8			/* R8 = &PGD[512] (physical) */
	LDA R2
	STA (R8)

	/*
	 * Fill PTE table: 1024 entries mapping physical 0-4MB.
	 * Each PTE = physical_page_addr | Present | Writable
	 */
	LDA #init_pte_table
	VIRT_TO_PHYS
	STA R0			/* R0 = PTE table base (physical) */
	LDA #3			/* R2 = phys page 0 | P | W */
	STA R2
	LDA #1024		/* R3 = counter */
	STA R3
3:
	LDA R2
	STA (R0)		/* Write PTE */
	LDA R2
	CLC
	ADC #0x1000		/* Next 4KB page */
	STA R2
	LDA R0
	CLC
	ADC #4			/* Next PTE slot */
	STA R0
	DEC R3
	LDA R3
	BNE 3b

	/*
	 * Set page table base register (PTBR).
	 * PTBR_LO = physical address of PGD.
	 * System registers at 0xFFFFF0xx bypass MMU.
	 */
	LDA #0xFFFFF014
	STA R8
	LDA #init_pg_dir
	VIRT_TO_PHYS
	STA (R8)		/* PTBR_LO = PGD physical addr */

	LDA #0xFFFFF018
	STA R8
	LDA #0
	STA (R8)		/* PTBR_HI = 0 */

	RTS
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
	.align 12
init_pg_dir:
	.space	4096		/* 1024 PGD entries */

init_pte_table:
	.space	4096		/* 1024 PTE entries */

	.align 12
init_stack:
	.space	8192		/* 8KB initial stack */
init_stack_top:
