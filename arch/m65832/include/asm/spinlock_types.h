/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Spinlock type definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_SPINLOCK_TYPES_H
#define _ASM_M65832_SPINLOCK_TYPES_H

#ifndef __LINUX_SPINLOCK_TYPES_RAW_H
# error "please don't include this file directly"
#endif

/*
 * Ticket spinlock type
 *
 * Layout:
 *   bits 0-15:  owner (current holder's ticket)
 *   bits 16-31: next (next ticket to be issued)
 */
typedef struct {
	volatile unsigned int lock;
} arch_spinlock_t;

#define __ARCH_SPIN_LOCK_UNLOCKED	{ 0 }

/*
 * Read-write spinlock type - use generic queued rwlock
 */
#include <asm-generic/qrwlock_types.h>

#endif /* _ASM_M65832_SPINLOCK_TYPES_H */
