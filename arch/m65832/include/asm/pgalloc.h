/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Page table allocation for the M65832 architecture.
 */

#ifndef _ASM_M65832_PGALLOC_H
#define _ASM_M65832_PGALLOC_H

#include <linux/mm.h>
#include <asm/pgtable.h>

/*
 * Allocate and free page directory entries
 */
static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *pgd = (pgd_t *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
	return pgd;
}

static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
	free_page((unsigned long)pgd);
}

/*
 * Populate PGD entry with PTE table
 */
static inline void pgd_populate(struct mm_struct *mm, pgd_t *pgd, pte_t *pte)
{
	*pgd = __pgd(__pa(pte) | _PAGE_TABLE);
}

/*
 * PTE table allocation handled in pgtable.h
 */

#define pmd_pgtable(pmd)	pmd_page(pmd)

#include <asm-generic/pgalloc.h>

#endif /* _ASM_M65832_PGALLOC_H */
