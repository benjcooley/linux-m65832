/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Page table definitions for the M65832 architecture.
 *
 * M65832 uses a 2-level page table:
 *   - PGD (Page Global Directory): 1024 entries, each pointing to a PTE table
 *   - PTE (Page Table Entry): 1024 entries, each mapping a 4KB page
 *
 * Virtual address breakdown (32-bit):
 *   [31:22] - PGD index (10 bits, 1024 entries)
 *   [21:12] - PTE index (10 bits, 1024 entries)
 *   [11:0]  - Page offset (12 bits, 4KB)
 */

#ifndef _ASM_M65832_PGTABLE_H
#define _ASM_M65832_PGTABLE_H

#include <asm/page.h>
#include <asm/mmu.h>
#include <asm-generic/pgtable-nopmd.h>

/*
 * Page table dimensions
 *
 * M65832 uses 2-level page tables with 64-bit entries:
 * - 1024 entries per table × 8 bytes = 8KB per table (2 pages)
 * - Virtual address: [31:22] PGD index, [21:12] PTE index, [11:0] offset
 */
#define PGDIR_SHIFT		22
#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE - 1))

#define PTRS_PER_PGD		1024
#define PTRS_PER_PTE		1024

/* Each table is 8KB (1024 × 8 bytes), requires 2 pages */
#define PGD_ORDER		1	/* PGD is 2 pages (8KB) */
#define PTE_ORDER		1	/* PTE table is 2 pages (8KB) */

/*
 * Page table entry bits (64-bit PTE)
 *
 * Bit 63:    NX (No Execute)
 * Bits 62:12: Physical Page Number (52 bits for 64-bit PA)
 * Bit 11:    Global - entry not flushed on ASID change
 * Bit 10:    Dirty - set by hardware on write
 * Bit 9:     Accessed - set by hardware on access
 * Bits 8-5:  Reserved
 * Bit 4:     PCD (Page Cache Disable)
 * Bit 3:     PWT (Page Write-Through)
 * Bit 2:     User - 1 = user accessible
 * Bit 1:     Writable - 1 = read/write
 * Bit 0:     Present - 1 = valid entry
 */
#define _PAGE_PRESENT		(1ULL << 0)
#define _PAGE_WRITE		(1ULL << 1)
#define _PAGE_USER		(1ULL << 2)
#define _PAGE_PWT		(1ULL << 3)	/* Write-through */
#define _PAGE_PCD		(1ULL << 4)	/* Cache disable */
#define _PAGE_ACCESSED		(1ULL << 9)	/* Set by hardware on access */
#define _PAGE_DIRTY		(1ULL << 10)	/* Set by hardware on write */
#define _PAGE_GLOBAL		(1ULL << 11)	/* Global (ignore ASID) */
#define _PAGE_SPECIAL		(1ULL << 8)	/* Use reserved bit for special */
#define _PAGE_NO_EXEC		(1ULL << 63)	/* No execute */

/* Combined permission bits */
#define _PAGE_TABLE		(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_USER)
#define _PAGE_CHG_MASK		(_PAGE_ACCESSED | _PAGE_DIRTY | _PAGE_SPECIAL)

/* PFN mask - physical frame number (bits 62:12, avoiding NX bit) */
#define _PFN_MASK		(0x7FFFFFFFFFFFF000ULL)

#ifndef __ASSEMBLY__

/*
 * Kernel page table (swapper_pg_dir)
 */
extern pgd_t swapper_pg_dir[PTRS_PER_PGD];

/*
 * Zero page for COW
 */
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)];
#define ZERO_PAGE(vaddr)	(virt_to_page(empty_zero_page))

/*
 * Page protection values
 * Note: No _PAGE_NO_EXEC means page is executable
 */
#define PAGE_NONE		__pgprot(0)
#define PAGE_SHARED		__pgprot(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_USER)
#define PAGE_COPY		__pgprot(_PAGE_PRESENT | _PAGE_USER | _PAGE_NO_EXEC)
#define PAGE_READONLY		__pgprot(_PAGE_PRESENT | _PAGE_USER | _PAGE_NO_EXEC)
#define PAGE_KERNEL		__pgprot(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_GLOBAL | _PAGE_NO_EXEC)
#define PAGE_KERNEL_RO		__pgprot(_PAGE_PRESENT | _PAGE_GLOBAL | _PAGE_NO_EXEC)
#define PAGE_KERNEL_EXEC	__pgprot(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_GLOBAL)
#define PAGE_KERNEL_NOCACHE	__pgprot(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_PCD | _PAGE_GLOBAL | _PAGE_NO_EXEC)

/*
 * PGD operations
 */
#define pgd_index(addr)		(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))
#define pgd_offset(mm, addr)	((mm)->pgd + pgd_index(addr))
#define pgd_offset_k(addr)	(swapper_pg_dir + pgd_index(addr))

static inline int pgd_none(pgd_t pgd)
{
	return pgd_val(pgd) == 0;
}

static inline int pgd_bad(pgd_t pgd)
{
	return 0;
}

static inline int pgd_present(pgd_t pgd)
{
	return pgd_val(pgd) & _PAGE_PRESENT;
}

static inline void pgd_clear(pgd_t *pgdp)
{
	*pgdp = __pgd(0);
}

/*
 * PTE operations
 */
#define pte_index(addr)		(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

static inline pte_t *pte_offset_kernel(pgd_t *pgd, unsigned long addr)
{
	return (pte_t *)__va(pgd_val(*pgd) & _PFN_MASK) + pte_index(addr);
}

#define pte_offset_map(dir, addr)	pte_offset_kernel((pgd_t *)(dir), addr)
#define pte_unmap(pte)			do { } while (0)

static inline int pte_none(pte_t pte)
{
	return pte_val(pte) == 0;
}

static inline int pte_present(pte_t pte)
{
	return pte_val(pte) & _PAGE_PRESENT;
}

static inline int pte_write(pte_t pte)
{
	return pte_val(pte) & _PAGE_WRITE;
}

static inline int pte_dirty(pte_t pte)
{
	return pte_val(pte) & _PAGE_DIRTY;
}

static inline int pte_young(pte_t pte)
{
	return pte_val(pte) & _PAGE_ACCESSED;
}

static inline int pte_special(pte_t pte)
{
	return pte_val(pte) & _PAGE_SPECIAL;
}

/*
 * PTE modification
 */
static inline pte_t pte_wrprotect(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_WRITE);
}

static inline pte_t pte_mkwrite_novma(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_WRITE);
}

static inline pte_t pte_mkclean(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_DIRTY);
}

static inline pte_t pte_mkdirty(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_DIRTY);
}

static inline pte_t pte_mkold(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_ACCESSED);
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_ACCESSED);
}

static inline pte_t pte_mkspecial(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_SPECIAL);
}

/*
 * PFN and page conversions
 * Note: We use 64-bit arithmetic for PTE values
 */
#define pte_pfn(pte)		(unsigned long)(((pte_val(pte) & _PFN_MASK) >> PAGE_SHIFT))
#define pfn_pte(pfn, prot)	__pte((((unsigned long long)(pfn)) << PAGE_SHIFT) | pgprot_val(prot))
#define pte_page(pte)		pfn_to_page(pte_pfn(pte))

#define mk_pte(page, prot)	pfn_pte(page_to_pfn(page), prot)

/*
 * Set PTE
 */
static inline void set_pte(pte_t *ptep, pte_t pte)
{
	*ptep = pte;
}

static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pte)
{
	set_pte(ptep, pte);
}

#define pte_clear(mm, addr, ptep)	set_pte(ptep, __pte(0))

/*
 * Page table allocation
 * Each table requires 2 pages (8KB) for 1024 × 8-byte entries
 */
#define pte_alloc_one_kernel(mm)	\
	((pte_t *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, PTE_ORDER))
#define pte_free_kernel(mm, pte)	free_pages((unsigned long)(pte), PTE_ORDER)

static inline pgtable_t pte_alloc_one(struct mm_struct *mm)
{
	struct page *page = alloc_pages(GFP_KERNEL | __GFP_ZERO, PTE_ORDER);
	if (!page)
		return NULL;
	return page;
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
	__free_pages(pte, PTE_ORDER);
}

/*
 * Update accessed/dirty bits
 */
static inline int ptep_test_and_clear_young(struct vm_area_struct *vma,
					    unsigned long addr, pte_t *ptep)
{
	pte_t pte = *ptep;
	if (!pte_young(pte))
		return 0;
	set_pte(ptep, pte_mkold(pte));
	return 1;
}

static inline pte_t ptep_get_and_clear(struct mm_struct *mm,
				       unsigned long addr, pte_t *ptep)
{
	pte_t pte = *ptep;
	pte_clear(mm, addr, ptep);
	return pte;
}

/*
 * Encode/decode swap entries
 */
#define __swp_type(x)		(((x).val >> 1) & 0x1f)
#define __swp_offset(x)		((x).val >> 6)
#define __swp_entry(type, off)	((swp_entry_t) { ((type) << 1) | ((off) << 6) })
#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)	__pte((x).val)

#define kern_addr_valid(addr)	(1)

#include <asm-generic/pgtable.h>

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_PGTABLE_H */
