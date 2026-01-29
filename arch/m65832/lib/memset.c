// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Memory set implementation for the M65832 architecture.
 */

#include <linux/types.h>
#include <linux/export.h>

/*
 * Set memory to a value - optimized version for M65832
 *
 * TODO: Optimize with M65832 block fill instructions if available
 */
void *memset(void *s, int c, size_t n)
{
	unsigned char *p = s;
	unsigned char val = (unsigned char)c;
	size_t i;

	/* Word-aligned fill for better performance */
	if (((unsigned long)p & 3) == 0 && n >= 4) {
		unsigned long *pl = (unsigned long *)p;
		unsigned long word = val | (val << 8) | (val << 16) | (val << 24);
		size_t words = n / 4;

		for (i = 0; i < words; i++)
			pl[i] = word;

		p = (unsigned char *)&pl[words];
		n &= 3;
	}

	/* Fill remaining bytes */
	for (i = 0; i < n; i++)
		p[i] = val;

	return s;
}
EXPORT_SYMBOL(memset);
