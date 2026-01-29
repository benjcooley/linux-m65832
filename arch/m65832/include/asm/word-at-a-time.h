/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Word-at-a-time helpers for little-endian M65832.
 */

#ifndef _ASM_M65832_WORD_AT_A_TIME_H
#define _ASM_M65832_WORD_AT_A_TIME_H

#include <linux/kernel.h>

/*
 * Little-endian word-at-a-time helpers
 */

struct word_at_a_time {
	const unsigned long one_bits, high_bits;
};

#define WORD_AT_A_TIME_CONSTANTS { 0x01010101UL, 0x80808080UL }

static inline unsigned long has_zero(unsigned long a, unsigned long *bits,
				     const struct word_at_a_time *c)
{
	unsigned long mask = ((a - c->one_bits) & ~a) & c->high_bits;
	*bits = mask;
	return mask;
}

static inline unsigned long prep_zero_mask(unsigned long a, unsigned long bits,
					   const struct word_at_a_time *c)
{
	return bits;
}

static inline unsigned long create_zero_mask(unsigned long bits)
{
	bits = (bits - 1) & ~bits;
	return bits >> 7;
}

#define zero_bytemask(mask) (mask)

static inline unsigned long find_zero(unsigned long mask)
{
	unsigned long ret = 0;

	if (mask >> 16)
		mask >>= 16, ret += 2;
	if (mask >> 8)
		mask >>= 8, ret += 1;
	return ret;
}

#endif /* _ASM_M65832_WORD_AT_A_TIME_H */
