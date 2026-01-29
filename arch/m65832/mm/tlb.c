// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * TLB management for the M65832 architecture.
 */

#include <linux/mm.h>
#include <linux/sched.h>

#include <asm/tlbflush.h>
#include <asm/mmu.h>
#include <asm/mmu_context.h>

/*
 * ASID allocation tracking
 */
unsigned long asid_generation = ASID_FIRST_VERSION;
DEFINE_SPINLOCK(asid_lock);

/*
 * Flush TLB entries for a range of pages being unmapped
 */
void tlb_flush(struct mmu_gather *tlb)
{
	if (tlb->fullmm)
		flush_tlb_mm(tlb->mm);
	else if (tlb->end > tlb->start)
		flush_tlb_range(tlb->vma, tlb->start, tlb->end);
}

/*
 * Called when a page table page is being freed
 */
void tlb_remove_table(struct mmu_gather *tlb, void *table)
{
	/* TLB entries pointing to this table are already flushed */
	free_page((unsigned long)table);
}
