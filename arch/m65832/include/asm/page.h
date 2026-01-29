/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Page definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_PAGE_H
#define _ASM_M65832_PAGE_H

#include <vdso/page.h>

/* Kernel virtual address start - upper 2GB */
#define PAGE_OFFSET	CONFIG_PAGE_OFFSET
#define KERNELBASE	PAGE_OFFSET

/* Physical memory offset */
#define PHYS_OFFSET	CONFIG_PHYS_OFFSET

#ifndef __ASSEMBLY__

#include <asm/setup.h>

#define clear_page(page)	memset((page), 0, PAGE_SIZE)
#define copy_page(to, from)	memcpy((to), (from), PAGE_SIZE)

#define clear_user_page(page, vaddr, pg)	clear_page(page)
#define copy_user_page(to, from, vaddr, pg)	copy_page(to, from)

/*
 * Page table entry types for type checking
 */
typedef struct {
	unsigned long pte;
} pte_t;

typedef struct {
	unsigned long pgd;
} pgd_t;

typedef struct {
	unsigned long pgprot;
} pgprot_t;

typedef struct page *pgtable_t;

#define pte_val(x)	((x).pte)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) })
#define __pgd(x)	((pgd_t) { (x) })
#define __pgprot(x)	((pgprot_t) { (x) })

/*
 * Virtual to physical address translation
 */
#define __va(x)		((void *)((unsigned long)(x) + PAGE_OFFSET - PHYS_OFFSET))
#define __pa(x)		((unsigned long)(x) - PAGE_OFFSET + PHYS_OFFSET)

static inline unsigned long virt_to_pfn(const void *kaddr)
{
	return __pa(kaddr) >> PAGE_SHIFT;
}

#define virt_to_page(addr) \
	(mem_map + (((unsigned long)(addr) - PAGE_OFFSET) >> PAGE_SHIFT))

#define page_to_virt(page) \
	((void *)(((page) - mem_map) << PAGE_SHIFT) + PAGE_OFFSET)

#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)

#define virt_addr_valid(kaddr)	(pfn_valid(virt_to_pfn(kaddr)))

#endif /* !__ASSEMBLY__ */

#include <asm-generic/memory_model.h>
#include <asm-generic/getorder.h>

#endif /* _ASM_M65832_PAGE_H */
