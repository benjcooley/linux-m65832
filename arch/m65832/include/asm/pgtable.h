/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Page table definitions for the M65832 architecture.
 *
 * M65832 uses a 2-level page table. In Linux's model, p4d/pud/pmd are folded.
 */

#ifndef _ASM_M65832_PGTABLE_H
#define _ASM_M65832_PGTABLE_H

#include <asm/page.h>
#include <asm/mmu.h>

/*
 * Page table dimensions for 2-level paging
 */
#define PGDIR_SHIFT		22
#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE - 1))

#define PTRS_PER_PGD		1024
#define PTRS_PER_PTE		1024

#define PGD_ORDER		1
#define PTE_ORDER		1

/* Include generic folding headers - folds p4d, pud, pmd into pgd */
#include <asm-generic/pgtable-nopmd.h>

/*
 * Page table entry bits (64-bit PTE)
 */
#define _PAGE_PRESENT		(1ULL << 0)
#define _PAGE_WRITE		(1ULL << 1)
#define _PAGE_USER		(1ULL << 2)
#define _PAGE_PWT		(1ULL << 3)
#define _PAGE_PCD		(1ULL << 4)
#define _PAGE_ACCESSED		(1ULL << 9)
#define _PAGE_DIRTY		(1ULL << 10)
#define _PAGE_GLOBAL		(1ULL << 11)
#define _PAGE_SPECIAL		(1ULL << 8)
#define _PAGE_NO_EXEC		(1ULL << 63)

#define _PAGE_TABLE		(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_USER)
#define _PAGE_CHG_MASK		(_PAGE_ACCESSED | _PAGE_DIRTY | _PAGE_SPECIAL)
#define _PAGE_KERN		(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_DIRTY | _PAGE_ACCESSED)

/* PFN mask */
#define _PFN_MASK		(0x7FFFFFFFFFFFF000ULL)

/* Shift for pfn in PTE */
#define PFN_PTE_SHIFT		PAGE_SHIFT

/* Page protection for ioremap */
#define _PAGE_IOREMAP		(_PAGE_PRESENT | _PAGE_WRITE | _PAGE_PCD | _PAGE_GLOBAL)

/*
 * vmalloc address range
 */
#define VMALLOC_START		0xD0000000UL
#define VMALLOC_END		0xF0000000UL

#ifndef __ASSEMBLY__

#define pgd_ERROR(e) \
	pr_err("%s:%d: bad pgd %08lx.\n", __FILE__, __LINE__, pgd_val(e))

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)];
#define ZERO_PAGE(vaddr)	(virt_to_page(empty_zero_page))

/*
 * Page protection values
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
 * PMD operations - our actual top-level directory (due to folding).
 * With nopmd folding, set_pgd -> set_p4d -> set_pud -> set_pmd,
 * so set_pmd is the one that actually writes the hardware page directory entry.
 */
static inline void set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	*pmdp = pmd;
}

#define pmd_none(x)		(!pmd_val(x))
#define pmd_bad(x)		((pmd_val(x) & (~PAGE_MASK & ~_PAGE_USER)) != _PAGE_KERN)
#define pmd_present(x)		(pmd_val(x) & _PAGE_PRESENT)
#define pmd_clear(xp)		do { set_pmd((xp), __pmd(0)); } while (0)

#define pmd_page(pmd)		(pfn_to_page(pmd_val(pmd) >> PAGE_SHIFT))
#define pmd_pfn(pmd)		(pmd_val(pmd) >> PAGE_SHIFT)

static inline unsigned long pmd_page_vaddr(pmd_t pmd)
{
	return (unsigned long)__va(pmd_val(pmd) & PAGE_MASK);
}

/*
 * PGD operations
 */
#define pgd_index(addr)		(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))
#define pgd_offset(mm, addr)	((mm)->pgd + pgd_index(addr))
/* pgd_offset_k is provided by linux/pgtable.h using pgd_offset(&init_mm, ...) */

/*
 * PTE operations
 * pte_index is provided by linux/pgtable.h
 */

/* pte_offset_kernel defined here, guards against generic definition */
#define pte_offset_kernel pte_offset_kernel
static inline pte_t *pte_offset_kernel(pmd_t *pmd, unsigned long addr)
{
	return (pte_t *)pmd_page_vaddr(*pmd) + ((addr >> PAGE_SHIFT) & (PTRS_PER_PTE - 1));
}

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
 */
#define pte_pfn(pte)		(unsigned long)(((pte_val(pte) & _PFN_MASK) >> PAGE_SHIFT))
#define pfn_pte(pfn, prot)	__pte((((unsigned long long)(pfn)) << PAGE_SHIFT) | pgprot_val(prot))
#define pte_page(pte)		pfn_to_page(pte_pfn(pte))

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
 * Page table allocation is handled by asm-generic/pgalloc.h
 * (included from asm/pgalloc.h).
 */

/*
 * Swap entry encoding
 */
#define __swp_type(x)		(((x).val >> 1) & 0x1f)
#define __swp_offset(x)		((x).val >> 6)
#define __swp_entry(type, off)	((swp_entry_t) { ((type) << 1) | ((off) << 6) })
#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)	__pte((x).val)

/*
 * Swap PTE exclusive bit (bit 1, same as _PAGE_WRITE, repurposed for swap)
 */
#define _PAGE_SWP_EXCLUSIVE	(1ULL << 1)

static inline int pte_swp_exclusive(pte_t pte)
{
	return pte_val(pte) & _PAGE_SWP_EXCLUSIVE;
}

static inline pte_t pte_swp_mkexclusive(pte_t pte)
{
	return __pte(pte_val(pte) | _PAGE_SWP_EXCLUSIVE);
}

static inline pte_t pte_swp_clear_exclusive(pte_t pte)
{
	return __pte(pte_val(pte) & ~_PAGE_SWP_EXCLUSIVE);
}

#define kern_addr_valid(addr)	(1)

/*
 * MMU cache update - no-op for M65832 (hardware manages TLB)
 */
#define update_mmu_cache(vma, addr, ptep) do { } while (0)
#define update_mmu_cache_range(vmf, vma, addr, ptep, nr) do { } while (0)

/*
 * Modify a PTE's protection bits
 */
static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	return __pte((pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot));
}

/*
 * Free a PTE page table page during TLB gather
 */
#define __pte_free_tlb(tlb, pte, addr)	pte_free((tlb)->mm, pte)

/*
 * arch_vma_access_permitted - always allow on M65832
 */
static inline bool arch_vma_access_permitted(struct vm_area_struct *vma,
					     bool write, bool execute,
					     bool foreign)
{
	return true;
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_PGTABLE_H */
