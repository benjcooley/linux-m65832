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
#include <asm-generic/bitops/lock.h>

/*
 * Arch-specific bitops workarounds for M65832 LLVM codegen bugs:
 *
 * 1) The generic __ffs/__fls binary-search pattern:
 *      if ((word & MASK) == 0) { num += N; word >>= N; }
 *    gets broken select/cmov lowering (num gets word value).
 *
 * 2) The ROL Rd,Rs,A instruction is broken (assembler/emulator
 *    encoding mismatch). The compiler emits ROL for ~(1 << n)
 *    patterns in __clear_bit/__change_bit.
 *
 * Fix: use lookup tables for bit scanning AND for bit masks,
 * avoiding both the select pattern and the ROL instruction.
 */

extern const unsigned char __ffs_byte_tab[256];
extern const unsigned char __fls_byte_tab[256];
extern const unsigned long __bit_mask_tab[32];

/*
 * Non-atomic bit operations using mask lookup table.
 * Avoids ROL instruction which is broken on M65832.
 */
#include <asm-generic/bitops/generic-non-atomic.h>

static __always_inline void
arch___set_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);
	*p |= __bit_mask_tab[nr % 32];
}

static __always_inline void
arch___clear_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);
	*p &= ~__bit_mask_tab[nr % 32];
}

static __always_inline void
arch___change_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);
	*p ^= __bit_mask_tab[nr % 32];
}

static __always_inline bool
arch___test_and_set_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long mask = __bit_mask_tab[nr % 32];
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);
	unsigned long old = *p;
	*p = old | mask;
	return (old & mask) != 0;
}

static __always_inline bool
arch___test_and_clear_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long mask = __bit_mask_tab[nr % 32];
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);
	unsigned long old = *p;
	*p = old & ~mask;
	return (old & mask) != 0;
}

static __always_inline bool
arch___test_and_change_bit(unsigned long nr, volatile unsigned long *addr)
{
	unsigned long mask = __bit_mask_tab[nr % 32];
	unsigned long *p = ((unsigned long *)addr) + (nr / 32);
	unsigned long old = *p;
	*p = old ^ mask;
	return (old & mask) != 0;
}

#define arch_test_bit generic_test_bit
#define arch_test_bit_acquire generic_test_bit_acquire

#include <asm-generic/bitops/non-instrumented-non-atomic.h>

/* __ffs: find first (least significant) set bit. Undefined if word == 0. */
#define __HAVE_ARCH___FFS
static __always_inline __attribute_const__ unsigned long __ffs(unsigned long word)
{
	if (word & 0xff)
		return __ffs_byte_tab[word & 0xff];
	if (word & 0xff00)
		return __ffs_byte_tab[(word >> 8) & 0xff] + 8;
	if (word & 0xff0000)
		return __ffs_byte_tab[(word >> 16) & 0xff] + 16;
	return __ffs_byte_tab[(word >> 24) & 0xff] + 24;
}

/* ffs: find first set bit, 1-indexed. Returns 0 if no bit set. */
#define __HAVE_ARCH_FFS
static __always_inline __attribute_const__ int ffs(int x)
{
	if (!x)
		return 0;
	return __ffs(x) + 1;
}

/* __fls: find last (most significant) set bit. Undefined if word == 0. */
#define __HAVE_ARCH___FLS
static __always_inline __attribute_const__ unsigned long __fls(unsigned long word)
{
	if (word & 0xff000000u)
		return __fls_byte_tab[(word >> 24) & 0xff] + 24;
	if (word & 0xff0000u)
		return __fls_byte_tab[(word >> 16) & 0xff] + 16;
	if (word & 0xff00u)
		return __fls_byte_tab[(word >> 8) & 0xff] + 8;
	return __fls_byte_tab[word & 0xff];
}

/* fls: find last set bit, 1-indexed. Returns 0 if no bit set. */
#define __HAVE_ARCH_FLS
static __always_inline __attribute_const__ int fls(unsigned int x)
{
	if (!x)
		return 0;
	return __fls(x) + 1;
}

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
