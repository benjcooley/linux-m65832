/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * TLB flush operations for the M65832 architecture.
 *
 * M65832 TLB features:
 * - 16-entry fully-associative TLB
 * - ASID tagging (8-bit, 256 address spaces)
 * - Selective invalidation by VA or ASID
 */

#ifndef _ASM_M65832_TLBFLUSH_H
#define _ASM_M65832_TLBFLUSH_H

#include <asm/mmu.h>

#ifndef __ASSEMBLY__

#include <linux/mm.h>

/*
 * TLB control register addresses
 */
#define TLB_INVAL_VA		((volatile unsigned long *)0xFFFFF004)
#define TLB_INVAL_ASID		((volatile unsigned long *)0xFFFFF00C)
#define TLB_FLUSH_ALL		((volatile unsigned long *)0xFFFFF01C)

/*
 * Flush entire TLB
 */
static inline void local_flush_tlb_all(void)
{
	/* Write any value to TLBFLUSH to flush entire TLB */
	*TLB_FLUSH_ALL = 0;
	/* Memory barrier to ensure TLB flush completes */
	asm volatile("FENCE" : : : "memory");
}

/*
 * Flush TLB for a specific virtual address
 */
static inline void local_flush_tlb_page(struct vm_area_struct *vma,
					unsigned long addr)
{
	/* Write the virtual address to TLBINVAL */
	*TLB_INVAL_VA = addr;
	asm volatile("FENCE" : : : "memory");
}

/*
 * Flush TLB for a range of virtual addresses
 */
static inline void local_flush_tlb_range(struct vm_area_struct *vma,
					 unsigned long start, unsigned long end)
{
	unsigned long addr;

	/* If range is large, just flush everything */
	if ((end - start) > (16 * PAGE_SIZE)) {
		local_flush_tlb_all();
		return;
	}

	/* Flush each page in the range */
	for (addr = start; addr < end; addr += PAGE_SIZE)
		*TLB_INVAL_VA = addr;

	asm volatile("FENCE" : : : "memory");
}

/*
 * Flush TLB for an entire mm (by ASID)
 */
static inline void local_flush_tlb_mm(struct mm_struct *mm)
{
	/* Write ASID to ASIDINVAL to flush all entries for this ASID */
	*TLB_INVAL_ASID = mm->context.asid;
	asm volatile("FENCE" : : : "memory");
}

/*
 * Flush kernel TLB entries
 */
static inline void local_flush_tlb_kernel_range(unsigned long start,
						unsigned long end)
{
	unsigned long addr;

	if ((end - start) > (16 * PAGE_SIZE)) {
		local_flush_tlb_all();
		return;
	}

	for (addr = start; addr < end; addr += PAGE_SIZE)
		*TLB_INVAL_VA = addr;

	asm volatile("FENCE" : : : "memory");
}

/*
 * Global TLB operations (for SMP - currently same as local)
 */
#define flush_tlb_all()			local_flush_tlb_all()
#define flush_tlb_mm(mm)		local_flush_tlb_mm(mm)
#define flush_tlb_page(vma, addr)	local_flush_tlb_page(vma, addr)
#define flush_tlb_range(vma, start, end) local_flush_tlb_range(vma, start, end)
#define flush_tlb_kernel_range(start, end) local_flush_tlb_kernel_range(start, end)

/*
 * Update TLB for a PTE change
 */
static inline void update_mmu_cache(struct vm_area_struct *vma,
				    unsigned long addr, pte_t *ptep)
{
	/* M65832 handles TLB refills in hardware on fault, no action needed */
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_TLBFLUSH_H */
