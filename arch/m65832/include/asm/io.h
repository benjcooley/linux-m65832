/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * I/O access definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_IO_H
#define _ASM_M65832_IO_H

#include <linux/types.h>
#include <asm/barrier.h>

/*
 * M65832 is memory-mapped I/O only - no port I/O
 */

#define IO_SPACE_LIMIT	0

/*
 * Memory-mapped I/O read functions
 */
static inline u8 __raw_readb(const volatile void __iomem *addr)
{
	return *(const volatile u8 __force *)addr;
}

static inline u16 __raw_readw(const volatile void __iomem *addr)
{
	return *(const volatile u16 __force *)addr;
}

static inline u32 __raw_readl(const volatile void __iomem *addr)
{
	return *(const volatile u32 __force *)addr;
}

/*
 * Memory-mapped I/O write functions
 */
static inline void __raw_writeb(u8 val, volatile void __iomem *addr)
{
	*(volatile u8 __force *)addr = val;
}

static inline void __raw_writew(u16 val, volatile void __iomem *addr)
{
	*(volatile u16 __force *)addr = val;
}

static inline void __raw_writel(u32 val, volatile void __iomem *addr)
{
	*(volatile u32 __force *)addr = val;
}

/*
 * Relaxed I/O - no memory barriers
 */
#define readb_relaxed	__raw_readb
#define readw_relaxed	__raw_readw
#define readl_relaxed	__raw_readl
#define writeb_relaxed	__raw_writeb
#define writew_relaxed	__raw_writew
#define writel_relaxed	__raw_writel

/*
 * Regular I/O - with memory barriers
 */
static inline u8 readb(const volatile void __iomem *addr)
{
	u8 val = __raw_readb(addr);
	rmb();
	return val;
}

static inline u16 readw(const volatile void __iomem *addr)
{
	u16 val = __raw_readw(addr);
	rmb();
	return val;
}

static inline u32 readl(const volatile void __iomem *addr)
{
	u32 val = __raw_readl(addr);
	rmb();
	return val;
}

static inline void writeb(u8 val, volatile void __iomem *addr)
{
	wmb();
	__raw_writeb(val, addr);
}

static inline void writew(u16 val, volatile void __iomem *addr)
{
	wmb();
	__raw_writew(val, addr);
}

static inline void writel(u32 val, volatile void __iomem *addr)
{
	wmb();
	__raw_writel(val, addr);
}

/*
 * String I/O operations
 */
static inline void readsb(const volatile void __iomem *addr, void *buf, unsigned long count)
{
	u8 *p = buf;
	while (count--)
		*p++ = __raw_readb(addr);
}

static inline void readsw(const volatile void __iomem *addr, void *buf, unsigned long count)
{
	u16 *p = buf;
	while (count--)
		*p++ = __raw_readw(addr);
}

static inline void readsl(const volatile void __iomem *addr, void *buf, unsigned long count)
{
	u32 *p = buf;
	while (count--)
		*p++ = __raw_readl(addr);
}

static inline void writesb(volatile void __iomem *addr, const void *buf, unsigned long count)
{
	const u8 *p = buf;
	while (count--)
		__raw_writeb(*p++, addr);
}

static inline void writesw(volatile void __iomem *addr, const void *buf, unsigned long count)
{
	const u16 *p = buf;
	while (count--)
		__raw_writew(*p++, addr);
}

static inline void writesl(volatile void __iomem *addr, const void *buf, unsigned long count)
{
	const u32 *p = buf;
	while (count--)
		__raw_writel(*p++, addr);
}

/*
 * Memory copy I/O
 */
static inline void memcpy_fromio(void *dest, const volatile void __iomem *src, size_t count)
{
	u8 *d = dest;
	const volatile u8 __iomem *s = src;
	while (count--)
		*d++ = __raw_readb(s++);
}

static inline void memcpy_toio(volatile void __iomem *dest, const void *src, size_t count)
{
	volatile u8 __iomem *d = dest;
	const u8 *s = src;
	while (count--)
		__raw_writeb(*s++, d++);
}

static inline void memset_io(volatile void __iomem *dest, int c, size_t count)
{
	volatile u8 __iomem *d = dest;
	while (count--)
		__raw_writeb(c, d++);
}

/*
 * I/O remapping - handled by mm/ioremap.c
 */
void __iomem *ioremap(phys_addr_t phys_addr, size_t size);
void iounmap(volatile void __iomem *addr);

#define ioremap_wc		ioremap
#define ioremap_wt		ioremap
#define ioremap_np		ioremap
#define ioremap_uc		ioremap

#include <asm-generic/io.h>

#endif /* _ASM_M65832_IO_H */
