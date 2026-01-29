/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * MMU context management for the M65832 architecture.
 */

#ifndef _ASM_M65832_MMU_CONTEXT_H
#define _ASM_M65832_MMU_CONTEXT_H

#include <linux/mm_types.h>
#include <linux/sched.h>

#include <asm/mmu.h>
#include <asm/tlbflush.h>

/*
 * ASID allocation
 */
#define ASID_FIRST_VERSION	(1 << ASID_BITS)
#define NUM_ASIDS		(1 << ASID_BITS)

extern unsigned long asid_generation;
extern unsigned long asid_map[NUM_ASIDS / (sizeof(unsigned long) * 8)];

/*
 * MMU control register access
 */
#define PTBR_LO		((volatile unsigned long *)0xFFFFF014)
#define PTBR_HI		((volatile unsigned long *)0xFFFFF018)
#define ASID_REG	((volatile unsigned long *)0xFFFFF008)

/*
 * Initialize a new context (called when mm_struct is created)
 */
static inline int init_new_context(struct task_struct *tsk,
				   struct mm_struct *mm)
{
	mm->context.asid = 0;
	mm->context.pgd_phys = __pa(mm->pgd);
	return 0;
}

/*
 * Destroy a context (called when mm_struct is destroyed)
 */
static inline void destroy_context(struct mm_struct *mm)
{
	/* Nothing to do - ASID will be reclaimed naturally */
}

/*
 * Allocate a new ASID for the given mm
 */
static inline unsigned long allocate_asid(struct mm_struct *mm)
{
	static unsigned long next_asid = 1;
	unsigned long asid;

	/* Simple round-robin ASID allocation */
	asid = next_asid++;
	if (next_asid >= NUM_ASIDS) {
		next_asid = 1;
		/* Flush TLB when we wrap around */
		local_flush_tlb_all();
	}

	return asid;
}

/*
 * Switch to a new MM context
 */
static inline void switch_mm(struct mm_struct *prev, struct mm_struct *next,
			     struct task_struct *tsk)
{
	unsigned long asid;

	if (prev == next)
		return;

	/* Allocate ASID if needed */
	asid = next->context.asid;
	if (asid == 0) {
		asid = allocate_asid(next);
		next->context.asid = asid;
	}

	/* Set the page table base register */
	*PTBR_LO = next->context.pgd_phys;
	*PTBR_HI = 0;	/* High bits for >4GB physical addresses */

	/* Set the ASID */
	*ASID_REG = asid;

	/* Memory barrier */
	asm volatile("fence" : : : "memory");
}

/*
 * Activate mm - called when entering userspace
 */
static inline void activate_mm(struct mm_struct *prev, struct mm_struct *next)
{
	switch_mm(prev, next, current);
}

/*
 * Deactivate mm - called when leaving userspace
 */
static inline void deactivate_mm(struct task_struct *tsk, struct mm_struct *mm)
{
	/* Nothing to do */
}

/*
 * Enter lazy TLB mode (kernel thread using mm)
 */
static inline void enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
	/* Nothing special needed */
}

#endif /* _ASM_M65832_MMU_CONTEXT_H */
