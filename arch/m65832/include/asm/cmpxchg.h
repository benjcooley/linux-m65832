/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Compare-and-exchange and exchange operations.
 *
 * M65832 has CAS (compare-and-swap) and LLI/SCI (load-linked/store-conditional)
 * but they only operate on DP/absolute addresses, not pointer-indirect.
 * For initial single-core bring-up, we use IRQ-disable based atomics.
 *
 * TODO: Use CAS/LLI/SCI for SMP when multi-core support is added.
 */

#ifndef _ASM_M65832_CMPXCHG_H
#define _ASM_M65832_CMPXCHG_H

#include <linux/types.h>
#include <linux/build_bug.h>
#include <asm/irqflags.h>

/*
 * Generic cmpxchg using IRQ disable (single-core safe)
 */
static inline unsigned long __cmpxchg_generic(volatile void *ptr,
					      unsigned long old,
					      unsigned long new,
					      int size)
{
	unsigned long flags, prev;

	arch_local_irq_save_flags_disable(&flags);

	switch (size) {
	case 1:
		prev = *(volatile unsigned char *)ptr;
		if (prev == (old & 0xff))
			*(volatile unsigned char *)ptr = new;
		break;
	case 2:
		prev = *(volatile unsigned short *)ptr;
		if (prev == (old & 0xffff))
			*(volatile unsigned short *)ptr = new;
		break;
	case 4:
		prev = *(volatile unsigned long *)ptr;
		if (prev == old)
			*(volatile unsigned long *)ptr = new;
		break;
	default:
		prev = 0;
		BUILD_BUG();
	}

	arch_local_irq_restore(flags);
	return prev;
}

#define arch_cmpxchg(ptr, o, n)						\
({									\
	(__typeof__(*(ptr)))(unsigned long)__cmpxchg_generic(		\
		(ptr),							\
		(unsigned long)(o),					\
		(unsigned long)(n),					\
		sizeof(*(ptr)));					\
})

/*
 * Generic xchg using IRQ disable (single-core safe)
 */
static inline unsigned long __xchg_generic(volatile void *ptr,
					   unsigned long val,
					   int size)
{
	unsigned long flags, prev;

	arch_local_irq_save_flags_disable(&flags);

	switch (size) {
	case 1:
		prev = *(volatile unsigned char *)ptr;
		*(volatile unsigned char *)ptr = val;
		break;
	case 2:
		prev = *(volatile unsigned short *)ptr;
		*(volatile unsigned short *)ptr = val;
		break;
	case 4:
		prev = *(volatile unsigned long *)ptr;
		*(volatile unsigned long *)ptr = val;
		break;
	default:
		prev = 0;
		BUILD_BUG();
	}

	arch_local_irq_restore(flags);
	return prev;
}

#define arch_xchg(ptr, v)						\
({									\
	(__typeof__(*(ptr)))(unsigned long)__xchg_generic(		\
		(ptr),							\
		(unsigned long)(v),					\
		sizeof(*(ptr)));					\
})

#endif /* _ASM_M65832_CMPXCHG_H */
