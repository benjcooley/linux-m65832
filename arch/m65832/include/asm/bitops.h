/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Bit operations for the M65832 architecture.
 */

#ifndef _ASM_M65832_BITOPS_H
#define _ASM_M65832_BITOPS_H

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <linux/compiler.h>
#include <asm/barrier.h>

/*
 * M65832 doesn't have dedicated bit manipulation instructions,
 * so we use generic C implementations with atomic protection.
 */

#include <asm-generic/bitops/atomic.h>
#include <asm-generic/bitops/non-atomic.h>
#include <asm-generic/bitops/lock.h>

/*
 * Find first set bit - use generic implementation
 * M65832 doesn't have ff1/ffs instruction
 */
#include <asm-generic/bitops/ffs.h>
#include <asm-generic/bitops/__ffs.h>
#include <asm-generic/bitops/ffz.h>
#include <asm-generic/bitops/fls.h>
#include <asm-generic/bitops/__fls.h>
#include <asm-generic/bitops/fls64.h>

/*
 * Bit counting
 */
#include <asm-generic/bitops/hweight.h>

/*
 * Little-endian bit operations
 */
#include <asm-generic/bitops/le.h>

/*
 * ext2 filesystem bit operations
 */
#include <asm-generic/bitops/ext2-atomic-setbit.h>

/*
 * Byte swap operations - M65832 is little-endian
 */
#include <asm-generic/bitops/sched.h>

#endif /* _ASM_M65832_BITOPS_H */
