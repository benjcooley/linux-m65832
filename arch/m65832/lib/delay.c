// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Delay functions for the M65832 architecture.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timex.h>

#include <asm/processor.h>

/*
 * Simple delay loop
 * Each iteration is approximately 4 cycles on M65832
 */
void __delay(unsigned long loops)
{
	asm volatile(
		"1:\n\t"
		"dec a\n\t"		/* Decrement counter */
		"bne 1b"		/* Loop if not zero */
		:
		: "a" (loops)
		: "memory"
	);
}
EXPORT_SYMBOL(__delay);

/*
 * Delay for a number of microseconds
 */
void __udelay(unsigned long usecs)
{
	unsigned long loops;

	/*
	 * loops = usecs * loops_per_jiffy * HZ / 1000000
	 * Simplified for integer math
	 */
	loops = usecs * loops_per_jiffy / (1000000 / HZ);
	__delay(loops);
}
EXPORT_SYMBOL(__udelay);

/*
 * Delay for a number of nanoseconds
 */
void __ndelay(unsigned long nsecs)
{
	unsigned long loops;

	loops = nsecs * loops_per_jiffy / (1000000000UL / HZ);
	if (loops < 1)
		loops = 1;
	__delay(loops);
}
EXPORT_SYMBOL(__ndelay);

/*
 * Constant delay - used when delay is known at compile time
 */
void __const_udelay(unsigned long xloops)
{
	unsigned long loops;

	loops = xloops * loops_per_jiffy;
	loops >>= 32;
	__delay(loops);
}
EXPORT_SYMBOL(__const_udelay);
