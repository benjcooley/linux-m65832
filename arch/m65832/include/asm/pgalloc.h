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
 * PGD requires 8KB (2 pages) for 1024 × 8-byte entries
 */
static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *pgd = (pgd_t *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, PGD_ORDER);
	return pgd;
}

static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
	free_pages((unsigned long)pgd, PGD_ORDER);
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
