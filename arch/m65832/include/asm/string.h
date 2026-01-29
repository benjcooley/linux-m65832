/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * String function declarations for the M65832 architecture.
 */

#ifndef _ASM_M65832_STRING_H
#define _ASM_M65832_STRING_H

/*
 * Use generic C implementations for now
 * These can be optimized with M65832 assembly later
 */

#define __HAVE_ARCH_MEMCPY
extern void *memcpy(void *dest, const void *src, size_t n);

#define __HAVE_ARCH_MEMSET
extern void *memset(void *s, int c, size_t n);

#define __HAVE_ARCH_MEMMOVE
extern void *memmove(void *dest, const void *src, size_t n);

#endif /* _ASM_M65832_STRING_H */
