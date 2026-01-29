/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Compare-and-exchange operations for the M65832 architecture.
 *
 * M65832 provides:
 * - CAS instruction: Compare-And-Swap (atomic compare and exchange)
 * - LLI/SCI: Load-Linked Indexed / Store-Conditional Indexed
 */

#ifndef _ASM_M65832_CMPXCHG_H
#define _ASM_M65832_CMPXCHG_H

#include <linux/types.h>
#include <asm/barrier.h>

/*
 * Compare and exchange for 32-bit values
 *
 * M65832 CAS instruction:
 * - Compares value at memory location with expected value
 * - If equal, stores new value and returns old value
 * - If not equal, returns current value without storing
 *
 * CAS A, addr - Compare A with [addr], if equal store X to [addr],
 *               always load [addr] to A
 */
static inline unsigned long __cmpxchg_u32(volatile unsigned long *ptr,
					  unsigned long old,
					  unsigned long new)
{
	unsigned long ret;

	/*
	 * M65832 CAS instruction:
	 * Input: A = expected old value, X = new value, address
	 * Output: A = actual old value (success if A unchanged)
	 */
	asm volatile(
		"lda %2\n\t"		/* Load expected value to A */
		"ldx %3\n\t"		/* Load new value to X */
		"cas %1\n\t"		/* CAS: if [ptr]==A then [ptr]=X; A=[ptr] */
		"sta %0"		/* Store result */
		: "=m" (ret), "+m" (*ptr)
		: "m" (old), "m" (new)
		: "memory"
	);

	return ret;
}

/*
 * Compare and exchange for 16-bit values
 */
static inline unsigned short __cmpxchg_u16(volatile unsigned short *ptr,
					   unsigned short old,
					   unsigned short new)
{
	unsigned short ret;

	asm volatile(
		"rep #$20\n\t"		/* Set 16-bit accumulator mode */
		"lda %2\n\t"
		"ldx %3\n\t"
		"cas %1\n\t"
		"sta %0\n\t"
		"rep #$00"		/* Restore 32-bit mode */
		: "=m" (ret), "+m" (*ptr)
		: "m" (old), "m" (new)
		: "memory"
	);

	return ret;
}

/*
 * Compare and exchange for 8-bit values
 */
static inline unsigned char __cmpxchg_u8(volatile unsigned char *ptr,
					 unsigned char old,
					 unsigned char new)
{
	unsigned char ret;

	asm volatile(
		"sep #$20\n\t"		/* Set 8-bit accumulator mode */
		"lda %2\n\t"
		"ldx %3\n\t"
		"cas %1\n\t"
		"sta %0\n\t"
		"rep #$20"		/* Restore 32-bit mode */
		: "=m" (ret), "+m" (*ptr)
		: "m" (old), "m" (new)
		: "memory"
	);

	return ret;
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
	unsigned long old;

	do {
		old = *ptr;
	} while (__cmpxchg_u32(ptr, old, new) != old);

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
