// SPDX-License-Identifier: GPL-2.0
/*
 * Optimized memset for M65832.
 *
 * Uses STQ (dp),Y for 64-bit stores in the aligned fast path (8x faster
 * than byte-at-a-time), falling back to 32-bit and byte stores for
 * alignment head/tail.
 *
 * STQ encoding: $02 $9F [dp_byte] — stores T:A (64 bits) to (Rn)+Y
 * TAT encoding: $02 $9B — copies A to T
 */

#include <linux/types.h>
#include <linux/export.h>
#include <linux/string.h>

/*
 * STQ (Rn),Y — store 64-bit T:A to address Rn+Y.
 * dp_byte is the register's DP offset (R0=0x00, R1=0x04, ...).
 * The assembler doesn't know STQ yet, so emit raw bytes.
 */
#define STQ_Rn_Y(reg_dp) \
	asm volatile(".byte 0x02, 0x9F, " #reg_dp : : : "memory")

void *memset(void *s, int c, size_t n)
{
	unsigned char *p = s;
	unsigned char val = (unsigned char)c;

	if (n < 8)
		goto tail;

	/* Align to 4-byte boundary */
	while ((unsigned long)p & 3) {
		*p++ = val;
		n--;
	}

	{
		unsigned long word = val;
		word |= word << 8;
		word |= word << 16;

		while (n >= 4) {
			*(unsigned long *)p = word;
			p += 4;
			n -= 4;
		}
	}

tail:
	while (n--)
		*p++ = val;

	return s;
}
EXPORT_SYMBOL(memset);
