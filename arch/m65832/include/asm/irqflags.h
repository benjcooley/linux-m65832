/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * IRQ flag manipulation for the M65832 architecture.
 */

#ifndef _ASM_M65832_IRQFLAGS_H
#define _ASM_M65832_IRQFLAGS_H

#include <asm/ptrace.h>

#ifndef __ASSEMBLY__

/*
 * Get current status register value
 * The status register is read via a special instruction or memory-mapped register
 */
static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;

	/*
	 * M65832: Read status register
	 * PHP pushes P to stack, then we pop it to a register
	 */
	asm volatile(
		"php\n\t"
		"pla\n\t"
		"sta %0"
		: "=m" (flags)
		:
		: "memory"
	);

	return flags;
}

/*
 * Disable local IRQs
 * SEI sets the I flag in the status register
 */
static inline void arch_local_irq_disable(void)
{
	asm volatile("sei" : : : "memory");
}

/*
 * Enable local IRQs
 * CLI clears the I flag in the status register
 */
static inline void arch_local_irq_enable(void)
{
	asm volatile("cli" : : : "memory");
}

/*
 * Disable IRQs and return previous state
 */
static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	flags = arch_local_save_flags();
	arch_local_irq_disable();

	return flags;
}

/*
 * Restore IRQ state
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	/*
	 * Restore the status register
	 * We only care about the I bit
	 */
	if (flags & SR_IRQ_DISABLE)
		arch_local_irq_disable();
	else
		arch_local_irq_enable();
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
