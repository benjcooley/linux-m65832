/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Delay function declarations for the M65832 architecture.
 */

#ifndef _ASM_M65832_DELAY_H
#define _ASM_M65832_DELAY_H

extern void __delay(unsigned long loops);
extern void __udelay(unsigned long usecs);
extern void __ndelay(unsigned long nsecs);
extern void __const_udelay(unsigned long xloops);

#define udelay(n)							\
({									\
	if (__builtin_constant_p(n)) {					\
		if ((n) < 20000)					\
			__const_udelay((n) * 0x10c7UL);			\
		else							\
			__udelay(n);					\
	} else {							\
		__udelay(n);						\
	}								\
})

#define ndelay(n)							\
({									\
	if (__builtin_constant_p(n)) {					\
		if ((n) < 20000)					\
			__const_udelay((n) * 5UL);			\
		else							\
			__ndelay(n);					\
	} else {							\
		__ndelay(n);						\
	}								\
})

#define mdelay(n)	({unsigned long __ms = (n); while (__ms--) udelay(1000);})

#endif /* _ASM_M65832_DELAY_H */
