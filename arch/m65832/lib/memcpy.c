// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Memory copy implementation for the M65832 architecture.
 */

#include <linux/types.h>
#include <linux/export.h>

/*
 * Copy memory - optimized version for M65832
 *
 * TODO: Optimize with M65832 block move instructions if available
 */
void *memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;
	size_t i;

	/* Word-aligned copy for better performance */
	if (((unsigned long)d & 3) == 0 && ((unsigned long)s & 3) == 0) {
		unsigned long *dl = (unsigned long *)d;
		const unsigned long *sl = (const unsigned long *)s;
		size_t words = n / 4;

		for (i = 0; i < words; i++)
			dl[i] = sl[i];

		d = (unsigned char *)&dl[words];
		s = (const unsigned char *)&sl[words];
		n &= 3;
	}

	/* Copy remaining bytes */
	for (i = 0; i < n; i++)
		d[i] = s[i];

	return dest;
}
EXPORT_SYMBOL(memcpy);

/*
 * Move memory - handles overlapping regions
 */
void *memmove(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	if (d == s)
		return dest;

	if (d < s) {
		/* Copy forward */
		while (n--)
			*d++ = *s++;
	} else {
		/* Copy backward */
		d += n;
		s += n;
		while (n--)
			*--d = *--s;
	}

	return dest;
}
EXPORT_SYMBOL(memmove);
