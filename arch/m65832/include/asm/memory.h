/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Memory layout definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_MEMORY_H
#define _ASM_M65832_MEMORY_H

#include <linux/const.h>
#include <asm/page.h>

/*
 * Physical memory layout
 */
#define PHYS_OFFSET		CONFIG_PHYS_OFFSET

/*
 * Kernel virtual memory layout
 *
 * 0x00000000 - 0x0000FFFF  Legacy 6502 area (reserved)
 * 0x00010000 - 0x7FFFFFFF  User space (~2GB)
 * 0x80000000 - 0xBFFFFFFF  Kernel direct-mapped (~1GB)
 * 0xC0000000 - 0xFFFFEFFF  vmalloc/ioremap
 * 0xFFFFF000 - 0xFFFFFFFF  Exception vectors, system registers
 */

/* Start of kernel virtual address space */
#define PAGE_OFFSET		CONFIG_PAGE_OFFSET

/* vmalloc area */
#define VMALLOC_START		_AC(0xC0000000, UL)
#define VMALLOC_END		_AC(0xFFFEFFFF, UL)

/* System register space (not accessible to normal mappings) */
#define M65832_SYSREG_START	_AC(0xFFFFF000, UL)
#define M65832_SYSREG_END	_AC(0xFFFFFFFF, UL)

/*
 * Conversion helpers
 */
#ifndef __ASSEMBLY__

#define __pa_symbol(x)		__pa(RELOC_HIDE((unsigned long)(x), 0))

static inline unsigned long __phys_to_virt(unsigned long x)
{
	return x + PAGE_OFFSET - PHYS_OFFSET;
}

static inline unsigned long __virt_to_phys(unsigned long x)
{
	return x - PAGE_OFFSET + PHYS_OFFSET;
}

#define __va(x)		((void *)__phys_to_virt((unsigned long)(x)))
#define __pa(x)		__virt_to_phys((unsigned long)(x))

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_MEMORY_H */
