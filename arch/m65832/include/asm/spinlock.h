/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Spinlock implementation for the M65832 architecture.
 *
 * Uses ticket locks implemented with CAS instruction.
 */

#ifndef _ASM_M65832_SPINLOCK_H
#define _ASM_M65832_SPINLOCK_H

#include <linux/spinlock_types.h>
#include <asm/barrier.h>
#include <asm/cmpxchg.h>
#include <asm/processor.h>

/*
 * Ticket spinlock - fair FIFO ordering
 *
 * The lock is split into two 16-bit values:
 * - owner: ticket number of current lock holder
 * - next: next ticket number to be issued
 */

#define TICKET_SHIFT	16

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned int tmp, ticket;

	/* Atomically get a ticket */
	do {
		tmp = READ_ONCE(lock->lock);
		ticket = tmp + (1 << TICKET_SHIFT);
	} while (arch_cmpxchg(&lock->lock, tmp, ticket) != tmp);

	/* Wait for our turn */
	ticket >>= TICKET_SHIFT;
	while ((lock->lock & 0xFFFF) != ticket)
		cpu_relax();

	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned int tmp, ticket;

	tmp = READ_ONCE(lock->lock);
	
	/* Check if lock is available (owner == next) */
	if (((tmp >> TICKET_SHIFT) & 0xFFFF) != (tmp & 0xFFFF))
		return 0;

	ticket = tmp + (1 << TICKET_SHIFT);
	if (arch_cmpxchg(&lock->lock, tmp, ticket) != tmp)
		return 0;

	smp_mb();
	return 1;
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	smp_mb();
	/* Increment the owner field */
	lock->lock++;
}

static inline int arch_spin_is_locked(arch_spinlock_t *lock)
{
	unsigned int tmp = READ_ONCE(lock->lock);
	return ((tmp >> TICKET_SHIFT) & 0xFFFF) != (tmp & 0xFFFF);
}

static inline int arch_spin_is_contended(arch_spinlock_t *lock)
{
	unsigned int tmp = READ_ONCE(lock->lock);
	return ((tmp >> TICKET_SHIFT) - tmp) > (1 << TICKET_SHIFT);
}

/*
 * Read-write spinlocks
 *
 * Use generic queued rwlocks
 */
#include <asm/qrwlock.h>

#endif /* _ASM_M65832_SPINLOCK_H */
