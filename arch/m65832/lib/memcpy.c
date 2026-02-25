// SPDX-License-Identifier: GPL-2.0
/*
 * Optimized memcpy / memmove for M65832.
 *
 * Uses 32-bit loads/stores for the aligned bulk (4x faster than byte).
 */

#include <linux/types.h>
#include <linux/export.h>
#include <linux/string.h>

void *memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	/*
	 * M65832 word loads/stores can fault on unaligned addresses.
	 * Only enter the 32-bit loop when both src and dest are aligned.
	 */
	while (n && (((unsigned long)d | (unsigned long)s) & 3)) {
		*d++ = *s++;
		n--;
	}

	if (n >= 4) {
		unsigned long *dl = (unsigned long *)d;
		const unsigned long *sl = (const unsigned long *)s;

		while (n >= 4) {
			*dl++ = *sl++;
			n -= 4;
		}

		d = (unsigned char *)dl;
		s = (const unsigned char *)sl;
	}
	while (n--)
		*d++ = *s++;

	return dest;
}
EXPORT_SYMBOL(memcpy);

void *memmove(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	if (d == s || n == 0)
		return dest;

	if (d < s || d >= s + n)
		return memcpy(dest, src, n);

	/* Backward copy for overlap */
	d += n;
	s += n;
	while (n--)
		*--d = *--s;

	return dest;
}
EXPORT_SYMBOL(memmove);
