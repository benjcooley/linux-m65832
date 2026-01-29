/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Cache flushing for the M65832 architecture.
 */

#ifndef _ASM_M65832_CACHEFLUSH_H
#define _ASM_M65832_CACHEFLUSH_H

#include <asm/barrier.h>

/*
 * M65832 cache operations
 *
 * TODO: Implement actual cache operations if M65832 has data/instruction caches
 * For now, use memory barriers as placeholders
 */

/* Flush data cache for a virtual address range */
static inline void flush_dcache_range(unsigned long start, unsigned long end)
{
	mb();
}

/* Flush instruction cache for a virtual address range */
static inline void flush_icache_range(unsigned long start, unsigned long end)
{
	mb();
}

/* Flush data cache for a page */
static inline void flush_dcache_page(struct page *page)
{
	mb();
}

/* Called when a page is mapped into user space */
#define flush_dcache_mmap_lock(mapping)		do { } while (0)
#define flush_dcache_mmap_unlock(mapping)	do { } while (0)

/* Flush caches before changing page protections */
#define flush_cache_all()		do { } while (0)
#define flush_cache_mm(mm)		do { } while (0)
#define flush_cache_dup_mm(mm)		do { } while (0)
#define flush_cache_range(vma, start, end)	do { } while (0)
#define flush_cache_page(vma, vmaddr, pfn)	do { } while (0)

/* Flush the icache for the page so the instruction changes are visible */
#define flush_icache_page(vma, page)	do { } while (0)
#define flush_icache_user_page(vma, page, addr, len)	do { } while (0)

/* Copy to userspace with cache handling */
#define copy_to_user_page(vma, page, vaddr, dst, src, len) \
	do {						\
		memcpy(dst, src, len);			\
		flush_icache_range((unsigned long)dst,	\
				   (unsigned long)(dst) + len); \
	} while (0)

#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)

#include <asm-generic/cacheflush.h>

#endif /* _ASM_M65832_CACHEFLUSH_H */
