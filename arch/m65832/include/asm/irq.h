/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * IRQ definitions for the M65832 architecture.
 */

#ifndef _ASM_M65832_IRQ_H
#define _ASM_M65832_IRQ_H

#include <asm-generic/irq.h>

#define NR_IRQS		32

/* M65832 interrupt sources */
#define IRQ_GPU_FRAME		0	/* GPU frame complete */
#define IRQ_GPU_CMD		1	/* GPU command buffer empty */
#define IRQ_DMA			2	/* DMA transfer complete */
#define IRQ_AUDIO		3	/* Audio buffer low */
#define IRQ_DISPLAY		4	/* Display vsync */
#define IRQ_TIMER0		5	/* Timer 0 */
#define IRQ_TIMER1		6	/* Timer 1 */
#define IRQ_UART		7	/* UART RX ready */
#define IRQ_SPI			8	/* SPI complete */
#define IRQ_I2C			9	/* I2C complete */
#define IRQ_GPIO		10	/* GPIO edge detected */
#define IRQ_SD			11	/* SD card ready */

void __init init_IRQ(void);

#endif /* _ASM_M65832_IRQ_H */
