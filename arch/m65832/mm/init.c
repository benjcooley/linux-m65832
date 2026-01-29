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

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/mmu.h>
#include <asm/setup.h>
#include <asm/sections.h>

/*
 * Kernel page directory (swapper_pg_dir)
 */
pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned_bss;

/*
 * Empty zero page for COW
 */
unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)] __page_aligned_bss;
EXPORT_SYMBOL(empty_zero_page);

/*
 * Memory boundaries
 */
unsigned long max_low_pfn;
unsigned long min_low_pfn;
unsigned long max_pfn;

/*
 * Set up initial page tables for the kernel
 */
static void __init setup_kernel_pagetables(void)
{
	unsigned long vaddr, paddr;
	pgd_t *pgd;
	pte_t *pte;
	unsigned long kernel_end_pfn;

	early_printk("M65832: Setting up kernel page tables\n");

	/* Clear swapper_pg_dir */
	memset(swapper_pg_dir, 0, sizeof(swapper_pg_dir));

	/*
	 * Map kernel text, data, BSS
	 * Virtual: PAGE_OFFSET + offset -> Physical: offset
	 */
	kernel_end_pfn = PFN_UP(__pa(_end));

	for (paddr = 0; paddr < (kernel_end_pfn << PAGE_SHIFT); paddr += PAGE_SIZE) {
		vaddr = (unsigned long)__va(paddr);
		
		pgd = pgd_offset_k(vaddr);
		
		/* Allocate PTE table if needed */
		if (pgd_none(*pgd)) {
			pte = memblock_alloc(PAGE_SIZE, PAGE_SIZE);
			if (!pte)
				panic("Failed to allocate PTE table");
			memset(pte, 0, PAGE_SIZE);
			*pgd = __pgd(__pa(pte) | _PAGE_TABLE);
		}

		/* Map the page */
		pte = pte_offset_kernel(pgd, vaddr);
		*pte = pfn_pte(paddr >> PAGE_SHIFT, PAGE_KERNEL);
	}

	early_printk("M65832: Kernel mapped %lu pages\n", kernel_end_pfn);
}

/*
 * Initialize paging
 */
void __init paging_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES];
	unsigned long start_pfn, end_pfn;

	early_printk("M65832: Initializing paging\n");

	/* Set up page tables */
	setup_kernel_pagetables();

	/* Determine memory boundaries */
	start_pfn = PFN_UP(memblock_start_of_DRAM());
	end_pfn = PFN_DOWN(memblock_end_of_DRAM());

	min_low_pfn = start_pfn;
	max_low_pfn = end_pfn;
	max_pfn = end_pfn;

	/* Set up zones */
	memset(zones_size, 0, sizeof(zones_size));
	zones_size[ZONE_DMA] = end_pfn - start_pfn;

	/* Initialize memory zones */
	free_area_init(zones_size);

	early_printk("M65832: Memory: %luMB\n",
		     (max_pfn - min_low_pfn) >> (20 - PAGE_SHIFT));
}

/*
 * Memory initialization
 */
void __init mem_init(void)
{
	/* Free all bootmem */
	memblock_free_all();

	/* Calculate memory statistics */
	mem_init_print_info(NULL);
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
