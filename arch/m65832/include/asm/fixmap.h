/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_M65832_FIXMAP_H
#define _ASM_M65832_FIXMAP_H

#include <asm/page.h>

/*
 * M65832 doesn't use fixmap - all MMIO is directly accessible.
 * Provide minimal definitions to satisfy earlycon.
 */
enum fixed_addresses {
	FIX_EARLYCON_MEM_BASE,
	__end_of_fixed_addresses
};

#define FIXADDR_TOP	0xFFFFF000UL
#define FIXADDR_SIZE	(__end_of_fixed_addresses << PAGE_SHIFT)
#define FIXADDR_START	(FIXADDR_TOP - FIXADDR_SIZE)

#define __fix_to_virt(x)	(FIXADDR_TOP - ((x) << PAGE_SHIFT))
#define __virt_to_fix(x)	((FIXADDR_TOP - ((x) & PAGE_MASK)) >> PAGE_SHIFT)

static inline unsigned long fix_to_virt(const unsigned int idx)
{
	return __fix_to_virt(idx);
}

static inline unsigned long virt_to_fix(const unsigned long vaddr)
{
	return __virt_to_fix(vaddr);
}

/* set_fixmap_io is used by earlycon */
#define set_fixmap_io(idx, phys) /* no-op: direct MMIO access */
#define clear_fixmap(idx)        /* no-op */

#endif /* _ASM_M65832_FIXMAP_H */
