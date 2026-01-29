// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * I/O memory remapping for the M65832 architecture.
 */

#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/mm.h>

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

/*
 * Remap I/O memory into kernel address space
 */
void __iomem *ioremap(phys_addr_t phys_addr, size_t size)
{
	unsigned long offset, last_addr;
	struct vm_struct *area;
	void __iomem *addr;
	pgprot_t prot;

	/* Don't allow wrap-around */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	/* Offset within page */
	offset = phys_addr & ~PAGE_MASK;
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(size + offset);

	/* Allocate virtual address space */
	area = get_vm_area(size, VM_IOREMAP);
	if (!area)
		return NULL;

	addr = (void __iomem *)area->addr;

	/* Map pages - uncached for I/O */
	prot = PAGE_KERNEL_NOCACHE;

	if (ioremap_page_range((unsigned long)addr,
			       (unsigned long)addr + size,
			       phys_addr, prot)) {
		vunmap((void *)addr);
		return NULL;
	}

	return addr + offset;
}
EXPORT_SYMBOL(ioremap);

/*
 * Unmap I/O memory
 */
void iounmap(volatile void __iomem *addr)
{
	if (addr)
		vunmap((void *)((unsigned long)addr & PAGE_MASK));
}
EXPORT_SYMBOL(iounmap);
