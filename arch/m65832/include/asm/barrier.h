/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Memory barrier definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_BARRIER_H
#define _ASM_M65832_BARRIER_H

#ifndef __ASSEMBLY__

/*
 * M65832 memory barriers:
 * - FENCE  - Full memory barrier (read + write)
 * - FENCER - Read memory barrier
 * - FENCEW - Write memory barrier
 *
 * These are Extended ALU instructions with specific opcodes.
 */

/* Full memory barrier */
#define mb()	asm volatile("fence" : : : "memory")

/* Read memory barrier */
#define rmb()	asm volatile("fencer" : : : "memory")

/* Write memory barrier */
#define wmb()	asm volatile("fencew" : : : "memory")

/*
 * SMP barriers - same as regular barriers on M65832 since
 * it's currently single-core
 */
#define __smp_mb()	mb()
#define __smp_rmb()	rmb()
#define __smp_wmb()	wmb()

/*
 * Compiler barrier
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
