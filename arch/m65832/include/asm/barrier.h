/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Memory barrier definitions for the M65832 architecture.
 *
 * M65832 provides three barrier instructions:
 * - fence  : Full memory barrier (read + write)
 * - fencer : Read memory barrier
 * - fencew : Write memory barrier
 */

#ifndef _ASM_M65832_BARRIER_H
#define _ASM_M65832_BARRIER_H

#ifndef __ASSEMBLY__

/* Full memory barrier */
#define mb()	asm volatile("FENCE" : : : "memory")

/* Read memory barrier */
#define rmb()	asm volatile("FENCER" : : : "memory")

/* Write memory barrier */
#define wmb()	asm volatile("FENCEW" : : : "memory")

/*
 * SMP barriers - same as regular barriers on M65832 since
 * it's currently single-core
 */
#define __smp_mb()	mb()
#define __smp_rmb()	rmb()
#define __smp_wmb()	wmb()

/*
 * Compiler barrier - prevents compiler reordering
 */
#define barrier()	asm volatile("" : : : "memory")

/*
 * Read/write barriers for I/O - always use full barriers
 */
#define __iormb()	rmb()
#define __iowmb()	wmb()

#include <asm-generic/barrier.h>

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_BARRIER_H */
