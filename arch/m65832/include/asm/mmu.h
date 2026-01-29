/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * MMU definitions for the M65832 architecture.
 *
 * M65832 MMU features:
 * - 2-level page tables (PGD -> PTE)
 * - 4KB page size
 * - 32-bit virtual address, 65-bit physical address
 * - 16-entry fully-associative TLB with ASID
 * - 8-bit ASID (256 address spaces)
 */

#ifndef _ASM_M65832_MMU_H
#define _ASM_M65832_MMU_H

#ifndef __ASSEMBLY__

/*
 * MMU context - stored per mm_struct
 */
typedef struct {
	unsigned long asid;		/* Address Space ID (0-255) */
	unsigned long pgd_phys;		/* Physical address of PGD */
} mm_context_t;

/*
 * MMU control register addresses
 */
#define M65832_MMUCR		0xFFFFF000	/* MMU Control Register */
#define M65832_TLBINVAL		0xFFFFF004	/* TLB Invalidate */
#define M65832_ASID		0xFFFFF008	/* Address Space ID */
#define M65832_ASIDINVAL	0xFFFFF00C	/* ASID Invalidate */
#define M65832_FAULTVA		0xFFFFF010	/* Faulting Virtual Address */
#define M65832_PTBR_LO		0xFFFFF014	/* Page Table Base (low 32 bits) */
#define M65832_PTBR_HI		0xFFFFF018	/* Page Table Base (high 33 bits) */
#define M65832_TLBFLUSH		0xFFFFF01C	/* TLB Flush All */

/*
 * MMU control register bits
 */
#define MMUCR_ENABLE		(1 << 0)	/* Enable MMU */
#define MMUCR_FLUSH_TLB		(1 << 1)	/* Flush TLB (write-only) */

/*
 * ASID management
 */
#define ASID_BITS		8
#define ASID_MASK		((1 << ASID_BITS) - 1)
#define MAX_ASID		ASID_MASK

/*
 * Page table entry bits (64-bit PTE)
 */
#define PTE_PRESENT		(1ULL << 0)	/* Page present */
#define PTE_WRITABLE		(1ULL << 1)	/* Writable */
#define PTE_USER		(1ULL << 2)	/* User accessible */
#define PTE_ACCESSED		(1ULL << 5)	/* Accessed */
#define PTE_DIRTY		(1ULL << 6)	/* Dirty */
#define PTE_GLOBAL		(1ULL << 8)	/* Global (ignore ASID) */
#define PTE_NO_EXEC		(1ULL << 63)	/* No execute */

/* Physical address bits in PTE (bits 12-63, excluding flags) */
#define PTE_PFN_SHIFT		12
#define PTE_PFN_MASK		(~((1ULL << PTE_PFN_SHIFT) - 1) & ~PTE_NO_EXEC)

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_MMU_H */
