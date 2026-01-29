/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * IRQ flag manipulation for the M65832 architecture.
 *
 * Uses SEI (Set Interrupt disable) and CLI (Clear Interrupt disable)
 * instructions to control the I flag in the status register.
 */

#ifndef _ASM_M65832_IRQFLAGS_H
#define _ASM_M65832_IRQFLAGS_H

#include <asm/ptrace.h>

#ifndef __ASSEMBLY__

/*
 * Get current status register value
 * PHP pushes P to stack, then we pop it to a register
 */
static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;

	asm volatile(
		"PHP\n\t"
		"PLA\n\t"
		"STA %0"
		: "=r" (flags)
		:
		: "a"
	);

	return flags;
}

/*
 * Disable local IRQs
 * SEI sets the I flag in the status register
 */
static inline void arch_local_irq_disable(void)
{
	asm volatile("SEI" : : : "memory", "cc");
}

/*
 * Enable local IRQs
 * CLI clears the I flag in the status register
 */
static inline void arch_local_irq_enable(void)
{
	asm volatile("CLI" : : : "memory", "cc");
}

/*
 * Disable IRQs and return previous state
 */
static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	asm volatile(
		"PHP\n\t"
		"SEI\n\t"
		"PLA\n\t"
		"STA %0"
		: "=r" (flags)
		:
		: "a", "memory", "cc"
	);

	return flags;
}

/*
 * Restore IRQ state
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	/*
	 * If the I bit was set (IRQs disabled), keep them disabled.
	 * Otherwise enable them.
	 */
	asm volatile(
		"LDA %0\n\t"
		"AND #4\n\t"		/* Check I bit (bit 2) */
		"BNE .LKEEP_DISABLED\n\t"
		"CLI\n\t"
		"BRA .LDONE\n\t"
		".LKEEP_DISABLED:\n\t"
		"SEI\n\t"
		".LDONE:"
		:
		: "r" (flags)
		: "a", "memory", "cc"
	);
}

/*
 * Check if IRQs are disabled
 */
static inline bool arch_irqs_disabled_flags(unsigned long flags)
{
	return (flags & SR_IRQ_DISABLE) != 0;
}

static inline bool arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_IRQFLAGS_H */
