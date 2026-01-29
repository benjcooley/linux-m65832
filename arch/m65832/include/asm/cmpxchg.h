/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Compare-and-exchange operations for the M65832 architecture.
 *
 * M65832 provides:
 * - CAS instruction: Compare-And-Swap (atomic compare and exchange)
 * - LLI/SCI: Load-Linked Indexed / Store-Conditional Indexed
 *
 * CAS syntax: cas Rd, (Rs), Re, Rn
 *   - Compare *Rs with Re
 *   - If equal, store Rn to *Rs
 *   - Load original value to Rd
 *   - Sets Z flag if swap occurred
 */

#ifndef _ASM_M65832_CMPXCHG_H
#define _ASM_M65832_CMPXCHG_H

#include <linux/types.h>
#include <asm/barrier.h>

/*
 * Compare and exchange for 32-bit values
 *
 * Returns the original value at *ptr.
 * If original == old, then *ptr is set to new.
 */
static inline unsigned long __cmpxchg_u32(volatile unsigned long *ptr,
					  unsigned long old,
					  unsigned long new)
{
	unsigned long ret;

	/*
	 * M65832 CAS instruction:
	 * cas ret, (ptr), old, new
	 * - Compares *ptr with old
	 * - If equal, stores new to *ptr
	 * - Always loads original *ptr value to ret
	 */
	asm volatile(
		"CAS %0, (%1), %2, %3"
		: "=r" (ret)
		: "r" (ptr), "r" (old), "r" (new)
		: "memory", "cc"
	);

	return ret;
}

/*
 * Compare and exchange for 16-bit values
 * Note: M65832 CAS operates on 32-bit values, so we need to
 * do a read-modify-write with proper masking
 */
static inline unsigned short __cmpxchg_u16(volatile unsigned short *ptr,
					   unsigned short old,
					   unsigned short new)
{
	volatile unsigned long *aligned_ptr;
	unsigned long shift, mask;
	unsigned long old32, new32, cur, ret;

	/* Align to 32-bit boundary */
	aligned_ptr = (volatile unsigned long *)((unsigned long)ptr & ~3UL);
	shift = ((unsigned long)ptr & 2) * 8;
	mask = 0xFFFFUL << shift;

	do {
		cur = *aligned_ptr;
		if (((cur >> shift) & 0xFFFF) != old)
			return (cur >> shift) & 0xFFFF;

		old32 = cur;
		new32 = (cur & ~mask) | ((unsigned long)new << shift);
		ret = __cmpxchg_u32(aligned_ptr, old32, new32);
	} while (ret != old32);

	return old;
}

/*
 * Compare and exchange for 8-bit values
 */
static inline unsigned char __cmpxchg_u8(volatile unsigned char *ptr,
					 unsigned char old,
					 unsigned char new)
{
	volatile unsigned long *aligned_ptr;
	unsigned long shift, mask;
	unsigned long old32, new32, cur, ret;

	aligned_ptr = (volatile unsigned long *)((unsigned long)ptr & ~3UL);
	shift = ((unsigned long)ptr & 3) * 8;
	mask = 0xFFUL << shift;

	do {
		cur = *aligned_ptr;
		if (((cur >> shift) & 0xFF) != old)
			return (cur >> shift) & 0xFF;

		old32 = cur;
		new32 = (cur & ~mask) | ((unsigned long)new << shift);
		ret = __cmpxchg_u32(aligned_ptr, old32, new32);
	} while (ret != old32);

	return old;
}

/*
 * Generic cmpxchg that selects the right size
 */
#define arch_cmpxchg(ptr, o, n)						\
({									\
	__typeof__(*(ptr)) __ret;					\
	switch (sizeof(*(ptr))) {					\
	case 1:								\
		__ret = (__typeof__(*(ptr)))__cmpxchg_u8(		\
			(volatile unsigned char *)(ptr),		\
			(unsigned char)(o), (unsigned char)(n));	\
		break;							\
	case 2:								\
		__ret = (__typeof__(*(ptr)))__cmpxchg_u16(		\
			(volatile unsigned short *)(ptr),		\
			(unsigned short)(o), (unsigned short)(n));	\
		break;							\
	case 4:								\
		__ret = (__typeof__(*(ptr)))__cmpxchg_u32(		\
			(volatile unsigned long *)(ptr),		\
			(unsigned long)(o), (unsigned long)(n));	\
		break;							\
	default:							\
		__ret = 0;						\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

/*
 * Exchange (swap) operations using CAS loop
 */
static inline unsigned long __xchg_u32(volatile unsigned long *ptr,
				       unsigned long new)
{
	unsigned long old, ret;

	do {
		old = *ptr;
		ret = __cmpxchg_u32(ptr, old, new);
	} while (ret != old);

	return old;
}

static inline unsigned short __xchg_u16(volatile unsigned short *ptr,
					unsigned short new)
{
	unsigned short old;

	do {
		old = *ptr;
	} while (__cmpxchg_u16(ptr, old, new) != old);

	return old;
}

static inline unsigned char __xchg_u8(volatile unsigned char *ptr,
				      unsigned char new)
{
	unsigned char old;

	do {
		old = *ptr;
	} while (__cmpxchg_u8(ptr, old, new) != old);

	return old;
}

#define arch_xchg(ptr, v)						\
({									\
	__typeof__(*(ptr)) __ret;					\
	switch (sizeof(*(ptr))) {					\
	case 1:								\
		__ret = (__typeof__(*(ptr)))__xchg_u8(			\
			(volatile unsigned char *)(ptr),		\
			(unsigned char)(v));				\
		break;							\
	case 2:								\
		__ret = (__typeof__(*(ptr)))__xchg_u16(			\
			(volatile unsigned short *)(ptr),		\
			(unsigned short)(v));				\
		break;							\
	case 4:								\
		__ret = (__typeof__(*(ptr)))__xchg_u32(			\
			(volatile unsigned long *)(ptr),		\
			(unsigned long)(v));				\
		break;							\
	default:							\
		__ret = 0;						\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#endif /* _ASM_M65832_CMPXCHG_H */
