/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Timer definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_TIMEX_H
#define _ASM_M65832_TIMEX_H

#define CLOCK_TICK_RATE		CONFIG_M65832_TIMER_FREQ

typedef unsigned long cycles_t;

static inline cycles_t get_cycles(void)
{
	/* Read timer count register */
	return *(volatile unsigned long *)0xFFFFF048;
}

#define get_cycles get_cycles

#endif /* _ASM_M65832_TIMEX_H */
