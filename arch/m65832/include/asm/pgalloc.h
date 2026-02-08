/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Page table allocation for the M65832 architecture.
 *
 * With pgtable-nopmd folding, our 2-level table works as:
 *   PGD (folded) -> PMD (= PGD, the real top-level) -> PTE
 *
 * The folding headers define pgd_populate, pgd_alloc, pgd_free etc.
 * We only need to provide pmd_populate (which populates what is
 * physically our PGD entries to point to PTE tables).
 */

#ifndef _ASM_M65832_PGALLOC_H
#define _ASM_M65832_PGALLOC_H

#include <asm/pgtable.h>

/*
 * Populate a PMD entry with a PTE table address (kernel context).
 * Due to nopmd folding, a PMD entry IS a PGD entry.
 */
static inline void pmd_populate_kernel(struct mm_struct *mm,
				       pmd_t *pmd, pte_t *pte)
{
	set_pmd(pmd, __pmd(__pa(pte) | _PAGE_TABLE));
}

/*
 * Populate a PMD entry with a PTE table page (user context).
 */
static inline void pmd_populate(struct mm_struct *mm,
				pmd_t *pmd, pgtable_t pte_page)
{
	set_pmd(pmd, __pmd(page_to_phys(pte_page) | _PAGE_TABLE));
}

#define pmd_pgtable(pmd)	pmd_page(pmd)

#include <asm-generic/pgalloc.h>

/*
 * pgd_alloc - allocate a PGD.
 * Uses the generic __pgd_alloc with order 0 (single page).
 */
static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	return __pgd_alloc(mm, 0);
}
#define pgd_alloc pgd_alloc

#endif /* _ASM_M65832_PGALLOC_H */
