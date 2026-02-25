// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Memory initialization for the M65832 architecture.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/initrd.h>
#include <linux/swap.h>
#include <linux/export.h>

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/mmu.h>
#include <asm/setup.h>
#include <asm/sections.h>

/*
 * Page protection map.
 * Indexed by vm_flags & (VM_READ|VM_WRITE|VM_EXEC|VM_SHARED).
 */
static const pgprot_t protection_map[16] = {
	[VM_NONE]					= PAGE_NONE,
	[VM_READ]					= PAGE_READONLY,
	[VM_WRITE]					= PAGE_COPY,
	[VM_WRITE | VM_READ]				= PAGE_COPY,
	[VM_EXEC]					= PAGE_READONLY,
	[VM_EXEC | VM_READ]				= PAGE_READONLY,
	[VM_EXEC | VM_WRITE]				= PAGE_COPY,
	[VM_EXEC | VM_WRITE | VM_READ]			= PAGE_COPY,
	[VM_SHARED]					= PAGE_NONE,
	[VM_SHARED | VM_READ]				= PAGE_READONLY,
	[VM_SHARED | VM_WRITE]				= PAGE_SHARED,
	[VM_SHARED | VM_WRITE | VM_READ]		= PAGE_SHARED,
	[VM_SHARED | VM_EXEC]				= PAGE_READONLY,
	[VM_SHARED | VM_EXEC | VM_READ]		= PAGE_READONLY,
	[VM_SHARED | VM_EXEC | VM_WRITE]		= PAGE_SHARED,
	[VM_SHARED | VM_EXEC | VM_WRITE | VM_READ]	= PAGE_SHARED,
};
DECLARE_VM_GET_PAGE_PROT

/*
 * Kernel page directory.
 * Normally this is swapper_pg_dir; we redirect init_mm.pgd to
 * init_pg_dir (the live page table set up by head.S) in paging_init
 * so that ioremap / vmalloc operate on the active page table.
 */
pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned_bss;

/* init_pg_dir lives in head.S BSS — it is the live page table. */
extern pgd_t init_pg_dir[];

/*
 * Empty zero page for COW
 */
unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)] __page_aligned_bss;
EXPORT_SYMBOL(empty_zero_page);

/*
 * Memory boundaries - declared in mm/memblock.c, just use extern here
 */
extern unsigned long max_low_pfn;
extern unsigned long min_low_pfn;
extern unsigned long max_pfn;

/*
 * Initialize paging.
 *
 * head.S has already set up init_pg_dir with:
 *   - Identity map: 0x00000000-0x007FFFFF (PGD[0..1])
 *   - Peripheral identity map: 0x10000000-0x103FFFFF (PGD[64])
 *   - Kernel linear map: PAGE_OFFSET..+64MB (PGD[512..527])
 * and the PTBR points to init_pg_dir.
 *
 * Rather than building a duplicate page table in swapper_pg_dir and
 * switching PTBR (which is fragile and triggers emulator issues), we
 * redirect init_mm.pgd to point directly at init_pg_dir.  This way
 * pgd_offset_k() returns entries in the live page table, and any new
 * mappings created by ioremap_page_range / vmalloc take effect
 * immediately — no PTBR switch needed.
 *
 * TODO: Proper transition to swapper_pg_dir once the emulator's PTBR
 * switch path is debugged.  This will also reclaim init_pg_dir memory.
 */
void __init paging_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES];
	unsigned long start_pfn, end_pfn;

	pr_info("M65832: Initializing paging\n");

	/* Determine memory boundaries */
	start_pfn = PFN_UP(memblock_start_of_DRAM());
	end_pfn = PFN_DOWN(memblock_end_of_DRAM());

	min_low_pfn = start_pfn;
	max_low_pfn = end_pfn;
	max_pfn = end_pfn;

	/*
	 * Point init_mm.pgd at the live page table so that all kernel
	 * page-table operations (ioremap, vmalloc, etc.) go through
	 * the page table the MMU is actually reading.
	 */
	init_mm.pgd = init_pg_dir;
	pr_info("M65832: init_mm.pgd -> init_pg_dir (%p, phys %08lx)\n",
		init_pg_dir, __pa(init_pg_dir));

	/* Set up zone end PFNs (free_area_init takes max PFN per zone) */
	memset(zones_size, 0, sizeof(zones_size));
	zones_size[ZONE_DMA] = end_pfn;

	/* Initialize memory zones */
	free_area_init(zones_size);

	pr_info("M65832: Memory: %luMB\n",
		     (max_pfn - min_low_pfn) >> (20 - PAGE_SHIFT));
}

/*
 * Memory initialization
 * memblock_free_all() is called by mm_core_init() in mm/mm_init.c
 * before this function. We just do any arch-specific setup.
 */
void __init mem_init(void)
{
	/* Clear the zero page */
	memset((void *)empty_zero_page, 0, PAGE_SIZE);
}

/*
 * Free memory used during initialization
 */
void __init free_initmem(void)
{
	free_initmem_default(POISON_FREE_INITMEM);
}

#ifdef CONFIG_BLK_DEV_INITRD
void __init free_initrd_mem(unsigned long start, unsigned long end)
{
	free_reserved_area((void *)start, (void *)end, POISON_FREE_INITMEM,
			   "initrd");
}
#endif
