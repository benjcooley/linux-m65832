/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * Platform-specific memory map and peripheral definitions.
 * Based on Milo832 SoC System Bus Architecture.
 */

#ifndef _ASM_M65832_PLATFORM_H
#define _ASM_M65832_PLATFORM_H

/*
 * Memory Map Overview:
 *
 * 0x0000_0000 - 0x0000_FFFF : Boot ROM (64 KB)
 * 0x0001_0000 - 0x001F_FFFF : DDR - CPU Code/Data (2 MB)
 * 0x0020_0000 - 0x0FFF_FFFF : DDR - Shared (GPU buffers, FB)
 *
 * 0x1000_0000 - 0x100F_FFFF : Peripheral registers (uncached, MMIO)
 * 0x1010_0000 - 0x101F_FFFF : Debug Port / JTAG
 * 0x2000_0000 - 0x2FFF_FFFF : External peripheral expansion
 *
 * 0xFFFF_F000 - 0xFFFF_FFFF : MMU/System registers (bypass MMU)
 */

/*
 * Physical memory layout
 */
#define M65832_BOOT_ROM_BASE		0x00000000
#define M65832_BOOT_ROM_SIZE		0x00010000	/* 64 KB */

#define M65832_DDR_BASE			0x00010000
#define M65832_DDR_SIZE			0x0FFF0000	/* ~256 MB */

/*
 * Peripheral base addresses (all 4 KB regions)
 */
#define M65832_PERIPH_BASE		0x10000000

#define M65832_GPU_BASE			0x10000000	/* GPU Registers */
#define M65832_DMA_BASE			0x10001000	/* DMA Controller */
#define M65832_AUDIO_BASE		0x10002000	/* Audio (SID-like) */
#define M65832_VIDEO_BASE		0x10003000	/* Video/Display */
#define M65832_TIMER_BASE		0x10004000	/* Timer/Watchdog */
#define M65832_INTC_BASE		0x10005000	/* Interrupt Controller */
#define M65832_UART_BASE		0x10006000	/* UART */
#define M65832_SPI_BASE			0x10007000	/* SPI Controller */
#define M65832_I2C_BASE			0x10008000	/* I2C Controller */
#define M65832_GPIO_BASE		0x10009000	/* GPIO */
#define M65832_SD_BASE			0x1000A000	/* SD Card Controller */

#define M65832_PERIPH_SIZE		0x1000		/* Each peripheral is 4 KB */

/*
 * MMU/System registers (bypass MMU translation)
 * These addresses are detected by hardware and bypass paging.
 */
#define M65832_SYSREG_BASE		0xFFFFF000

#define M65832_MMUCR			0xFFFFF000	/* MMU Control Register */
#define M65832_TLBINVAL			0xFFFFF004	/* TLB Invalidate (write VA) */
#define M65832_ASID			0xFFFFF008	/* Address Space ID */
#define M65832_ASIDINVAL		0xFFFFF00C	/* Invalidate by ASID */
#define M65832_FAULTVA			0xFFFFF010	/* Faulting VA (read-only) */
#define M65832_PTBR_LO			0xFFFFF014	/* Page Table Base, low 32 */
#define M65832_PTBR_HI			0xFFFFF018	/* Page Table Base, high */
#define M65832_TLBFLUSH			0xFFFFF01C	/* Write 1 to flush TLB */

/* System timer (in system register space) */
#define M65832_SYSTIMER_CTRL		0xFFFFF040	/* Timer control */
#define M65832_SYSTIMER_CMP		0xFFFFF044	/* Timer compare */
#define M65832_SYSTIMER_COUNT		0xFFFFF048	/* Timer count */

/*
 * UART Register Offsets (simple 16550-like)
 */
#define UART_DATA			0x00	/* TX/RX data register */
#define UART_STATUS			0x04	/* Status register */
#define UART_CTRL			0x08	/* Control register */
#define UART_BAUD			0x0C	/* Baud rate divisor */

/* UART Status bits */
#define UART_STATUS_RXRDY		(1 << 0)	/* RX data ready */
#define UART_STATUS_TXRDY		(1 << 1)	/* TX ready for data */
#define UART_STATUS_RXFULL		(1 << 2)	/* RX FIFO full */
#define UART_STATUS_TXEMPTY		(1 << 3)	/* TX FIFO empty */
#define UART_STATUS_RXERR		(1 << 4)	/* RX error (framing/parity) */
#define UART_STATUS_TXBUSY		(1 << 5)	/* TX in progress */

/* UART Control bits */
#define UART_CTRL_RXIE			(1 << 0)	/* RX interrupt enable */
#define UART_CTRL_TXIE			(1 << 1)	/* TX interrupt enable */
#define UART_CTRL_ENABLE		(1 << 2)	/* UART enable */

/*
 * Interrupt Controller Register Offsets
 */
#define INTC_STATUS			0x00	/* Current status (read-only) */
#define INTC_ENABLE			0x04	/* Interrupt enable mask */
#define INTC_PENDING			0x08	/* STATUS & ENABLE (read-only) */
#define INTC_CLEAR			0x0C	/* Write 1 to clear edge IRQs */
#define INTC_PRIORITY			0x10	/* Priority of highest pending */

/*
 * SD Card Controller Register Offsets
 */
#define SD_CTRL				0x00	/* Control register */
#define SD_STATUS			0x04	/* Status register */
#define SD_CMD				0x08	/* Command register */
#define SD_ARG				0x0C	/* Command argument */
#define SD_RESP0			0x10	/* Response word 0 */
#define SD_RESP1			0x14	/* Response word 1 */
#define SD_RESP2			0x18	/* Response word 2 */
#define SD_RESP3			0x1C	/* Response word 3 */
#define SD_DATA				0x20	/* Data register */
#define SD_BLKSIZE			0x24	/* Block size */
#define SD_BLKCNT			0x28	/* Block count */
#define SD_TIMEOUT			0x2C	/* Timeout value */

/* SD Status bits */
#define SD_STATUS_PRESENT		(1 << 0)	/* Card present */
#define SD_STATUS_READY			(1 << 1)	/* Card ready */
#define SD_STATUS_BUSY			(1 << 2)	/* Transfer in progress */
#define SD_STATUS_ERROR			(1 << 3)	/* Error occurred */
#define SD_STATUS_COMPLETE		(1 << 4)	/* Transfer complete */

/*
 * Timer Register Offsets (system timer in SYSREG area)
 */
#define TIMER_CTRL			0x00
#define TIMER_CMP			0x04
#define TIMER_COUNT			0x08

/* Timer Control bits */
#define TIMER_CTRL_EN			(1 << 0)	/* Enable */
#define TIMER_CTRL_IE			(1 << 1)	/* Interrupt enable */
#define TIMER_CTRL_IF			(1 << 2)	/* Interrupt flag (W1C) */
#define TIMER_CTRL_PERIODIC		(1 << 3)	/* Periodic mode */

/*
 * Default clock frequencies
 */
#define M65832_CPU_CLOCK_HZ		50000000	/* 50 MHz default */
#define M65832_TIMER_CLOCK_HZ		50000000	/* Timer runs at CPU clock */
#define M65832_UART_CLOCK_HZ		50000000	/* UART base clock */

#endif /* _ASM_M65832_PLATFORM_H */
