/* SPDX-License-Identifier: GPL-2.0 */
/*
 * M65832 Linux
 *
 * IRQ flag manipulation for the M65832 architecture.
 *
 * Uses SEI (Set Interrupt disable) and CLI (Clear Interrupt disable)
 * instructions to control the I flag in the status register.
 *
 * Inline asm constraints:
 *   "r" = GPR (R0-R23), "a" = A accumulator,
 *   "x" = X register, "y" = Y register
 */

#ifndef _ASM_M65832_IRQFLAGS_H
#define _ASM_M65832_IRQFLAGS_H

#include <asm/ptrace.h>

#ifndef __ASSEMBLY__

/*
 * Get current status register value
 * PHP pushes P to stack, then PLA pops it to A
 */
static __always_inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;

	asm volatile(
		"PHP\n"
		"PLA\n"
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
static __always_inline void arch_local_irq_disable(void)
{
	asm volatile("SEI" : : : "memory", "cc");
}

/*
 * Enable local IRQs
 * CLI clears the I flag in the status register
 */
static __always_inline void arch_local_irq_enable(void)
{
	asm volatile("CLI" : : : "memory", "cc");
}

/*
 * Save flags and disable IRQs - used by cmpxchg.h
 * Separate function to avoid circular dependency with full irq_save
 */
static __always_inline void arch_local_irq_save_flags_disable(unsigned long *flags)
{
	asm volatile(
		"PHP\n"
		"SEI\n"
		"PLA\n"
		"STA %0"
		: "=r" (*flags)
		:
		: "a", "memory", "cc"
	);
}

/*
 * Disable IRQs and return previous state
 */
static __always_inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	arch_local_irq_save_flags_disable(&flags);

	return flags;
}

/*
 * Restore IRQ state
 * Uses C branching to avoid duplicate label issues when inlined
 */
static __always_inline void arch_local_irq_restore(unsigned long flags)
{
	if (flags & SR_IRQ_DISABLE)
		arch_local_irq_disable();
	else
		arch_local_irq_enable();
}

/*
 * Check if IRQs are disabled
 */
static __always_inline bool arch_irqs_disabled_flags(unsigned long flags)
{
	return (flags & SR_IRQ_DISABLE) != 0;
}

static __always_inline bool arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_M65832_IRQFLAGS_H */
