/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Atomic operations for the M65832 architecture.
 *
 * M65832 provides atomic operations via:
 * - CAS (Compare-And-Swap) instruction
 * - LLI/SCI (Load-Linked/Store-Conditional) instruction pair
 */

#ifndef _ASM_M65832_ATOMIC_H
#define _ASM_M65832_ATOMIC_H

#include <linux/types.h>
#include <asm/barrier.h>
#include <asm/cmpxchg.h>

#define ATOMIC_INIT(i)	{ (i) }

/*
 * Read atomic variable
 */
static inline int arch_atomic_read(const atomic_t *v)
{
	return READ_ONCE(v->counter);
}

/*
 * Write atomic variable
 */
static inline void arch_atomic_set(atomic_t *v, int i)
{
	WRITE_ONCE(v->counter, i);
}

/*
 * Add and return new value
 */
static inline int arch_atomic_add_return(int i, atomic_t *v)
{
	int old, new, ret;

	do {
		old = arch_atomic_read(v);
		new = old + i;
		ret = arch_cmpxchg(&v->counter, old, new);
	} while (ret != old);

	return new;
}

/*
 * Subtract and return new value
 */
static inline int arch_atomic_sub_return(int i, atomic_t *v)
{
	return arch_atomic_add_return(-i, v);
}

/*
 * Fetch and add (return old value)
 */
static inline int arch_atomic_fetch_add(int i, atomic_t *v)
{
	int old, new, ret;

	do {
		old = arch_atomic_read(v);
		new = old + i;
		ret = arch_cmpxchg(&v->counter, old, new);
	} while (ret != old);

	return old;
}

/*
 * Fetch and subtract (return old value)
 */
static inline int arch_atomic_fetch_sub(int i, atomic_t *v)
{
	return arch_atomic_fetch_add(-i, v);
}

/*
 * Fetch and bitwise AND
 */
static inline int arch_atomic_fetch_and(int i, atomic_t *v)
{
	int old, new, ret;

	do {
		old = arch_atomic_read(v);
		new = old & i;
		ret = arch_cmpxchg(&v->counter, old, new);
	} while (ret != old);

	return old;
}

/*
 * Fetch and bitwise OR
 */
static inline int arch_atomic_fetch_or(int i, atomic_t *v)
{
	int old, new, ret;

	do {
		old = arch_atomic_read(v);
		new = old | i;
		ret = arch_cmpxchg(&v->counter, old, new);
	} while (ret != old);

	return old;
}

/*
 * Fetch and bitwise XOR
 */
static inline int arch_atomic_fetch_xor(int i, atomic_t *v)
{
	int old, new, ret;

	do {
		old = arch_atomic_read(v);
		new = old ^ i;
		ret = arch_cmpxchg(&v->counter, old, new);
	} while (ret != old);

	return old;
}

/*
 * Exchange
 */
static inline int arch_atomic_xchg(atomic_t *v, int new)
{
	return arch_xchg(&v->counter, new);
}

/*
 * Compare and exchange
 */
static inline int arch_atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return arch_cmpxchg(&v->counter, old, new);
}

#define arch_atomic_add(i, v)		(void)arch_atomic_add_return(i, v)
#define arch_atomic_sub(i, v)		(void)arch_atomic_sub_return(i, v)
#define arch_atomic_inc(v)		arch_atomic_add(1, v)
#define arch_atomic_dec(v)		arch_atomic_sub(1, v)
#define arch_atomic_inc_return(v)	arch_atomic_add_return(1, v)
#define arch_atomic_dec_return(v)	arch_atomic_sub_return(1, v)

#define arch_atomic_and(i, v)		(void)arch_atomic_fetch_and(i, v)
#define arch_atomic_or(i, v)		(void)arch_atomic_fetch_or(i, v)
#define arch_atomic_xor(i, v)		(void)arch_atomic_fetch_xor(i, v)

/*
 * Atomic operations with explicit memory ordering
 */
#define arch_atomic_add_return_relaxed		arch_atomic_add_return
#define arch_atomic_sub_return_relaxed		arch_atomic_sub_return
#define arch_atomic_fetch_add_relaxed		arch_atomic_fetch_add
#define arch_atomic_fetch_sub_relaxed		arch_atomic_fetch_sub
#define arch_atomic_fetch_and_relaxed		arch_atomic_fetch_and
#define arch_atomic_fetch_or_relaxed		arch_atomic_fetch_or
#define arch_atomic_fetch_xor_relaxed		arch_atomic_fetch_xor
#define arch_atomic_xchg_relaxed		arch_atomic_xchg
#define arch_atomic_cmpxchg_relaxed		arch_atomic_cmpxchg

#include <asm-generic/atomic64.h>

#endif /* _ASM_M65832_ATOMIC_H */
