/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Cache definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_CACHE_H
#define _ASM_M65832_CACHE_H

/*
 * M65832 cache line size
 * TODO: Determine actual cache line size from hardware
 */
#define L1_CACHE_SHIFT		5
#define L1_CACHE_BYTES		(1 << L1_CACHE_SHIFT)

#define SMP_CACHE_BYTES		L1_CACHE_BYTES

/*
 * Cache alignment attribute
 */
#define __cacheline_aligned	__attribute__((__aligned__(L1_CACHE_BYTES)))
#define ____cacheline_aligned	__attribute__((__aligned__(L1_CACHE_BYTES)))

#endif /* _ASM_M65832_CACHE_H */
