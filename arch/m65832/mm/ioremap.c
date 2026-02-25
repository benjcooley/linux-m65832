// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * I/O memory remapping for the M65832 architecture.
 *
 * The initial page tables (init_pg_dir, set up by head.S) already
 * identity-map the peripheral region 0x10000000-0x103FFFFF and the
 * system registers at 0xFFFFF000 bypass the MMU entirely.
 *
 * Full vmalloc-based ioremap doesn't work yet because init_mm.pgd
 * points to swapper_pg_dir while the MMU still uses init_pg_dir.
 * Until we either switch the MMU to swapper_pg_dir or implement a
 * vmalloc fault handler, return direct identity-mapped addresses
 * for regions that are already mapped.
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
	unsigned long last_addr;

	/* Don't allow wrap-around or zero size */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	/*
	 * Peripherals at 0x10000000-0x103FFFFF are identity-mapped
	 * by head.S (init_pg_dir PGD[64]).  Return the physical
	 * address directly — the MMU already translates it 1:1.
	 */
	if (phys_addr >= 0x10000000 && last_addr <= 0x103FFFFF)
		return (void __iomem *)phys_addr;

	/*
	 * System registers at 0xFFFFF000-0xFFFFFFFF bypass the MMU
	 * entirely in hardware.  Return the address directly.
	 */
	if (phys_addr >= 0xFFFFF000)
		return (void __iomem *)phys_addr;

	pr_warn("ioremap: cannot map phys %pa size %zu (not in identity-mapped region)\n",
		&phys_addr, size);
	return NULL;
}
EXPORT_SYMBOL(ioremap);

/*
 * Unmap I/O memory
 */
void iounmap(volatile void __iomem *addr)
{
	/*
	 * Identity-mapped regions don't use vmalloc, so nothing
	 * to free.  Full vmalloc-based ioremap would vunmap here.
	 */
}
EXPORT_SYMBOL(iounmap);
