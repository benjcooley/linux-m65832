// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * IRQ handling for the M65832 architecture.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include <asm/ptrace.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/platform.h>

#if defined(CONFIG_M65832) && defined(__clang__)
#define M65832_IRQ_OPTNONE __attribute__((optnone))
#else
#define M65832_IRQ_OPTNONE
#endif

/*
 * M65832 Interrupt Controller registers
 * Use definitions from platform.h
 */

static void __iomem *intc_base;
static struct irq_domain *m65832_irq_domain;
static bool m65832_intc_disabled;

/* Avoid backend issues around variable shifts in IRQ mask paths. */
static const u32 m65832_irq_bit[32] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
};

/*
 * Number of IRQ sources
 */
#define M65832_NR_IRQS		32
static unsigned int m65832_virq_map[M65832_NR_IRQS];

/*
 * IRQ chip callbacks
 */
static M65832_IRQ_OPTNONE void m65832_irq_mask(struct irq_data *d)
{
	unsigned int irq = d->hwirq;
	unsigned long flags;
	u32 enable;
	u32 bit;

	if (!intc_base)
		return;
	if (irq >= M65832_NR_IRQS)
		return;
	bit = m65832_irq_bit[irq];

	local_irq_save(flags);
	enable = readl(intc_base + INTC_ENABLE);
	enable &= ~bit;
	writel(enable, intc_base + INTC_ENABLE);
	local_irq_restore(flags);
}

static M65832_IRQ_OPTNONE void m65832_irq_unmask(struct irq_data *d)
{
	unsigned int irq = d->hwirq;
	unsigned long flags;
	u32 enable;
	u32 bit;

	if (!intc_base)
		return;
	if (irq >= M65832_NR_IRQS)
		return;
	bit = m65832_irq_bit[irq];

	local_irq_save(flags);
	enable = readl(intc_base + INTC_ENABLE);
	enable |= bit;
	writel(enable, intc_base + INTC_ENABLE);
	local_irq_restore(flags);
}

static M65832_IRQ_OPTNONE void m65832_irq_ack(struct irq_data *d)
{
	unsigned int irq = d->hwirq;
	u32 bit;

	if (!intc_base || irq >= M65832_NR_IRQS)
		return;

	bit = m65832_irq_bit[irq];
	writel(bit, intc_base + INTC_CLEAR);
}

static struct irq_chip m65832_irq_chip = {
	.name		= "M65832-INTC",
	.irq_mask	= m65832_irq_mask,
	.irq_unmask	= m65832_irq_unmask,
	.irq_ack	= m65832_irq_ack,
};

/*
 * IRQ domain callbacks
 */
static int m65832_irq_domain_map(struct irq_domain *d, unsigned int irq,
				 irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler(irq, &m65832_irq_chip, handle_level_irq);
	irq_set_chip_data(irq, NULL);
	irq_set_noprobe(irq);
	return 0;
}

static const struct irq_domain_ops m65832_irq_domain_ops = {
	.map = m65832_irq_domain_map,
	.xlate = irq_domain_xlate_onecell,
};

/*
 * Initialize the interrupt controller
 */
void __init init_IRQ(void)
{
	pr_info("M65832: Initializing interrupt controller\n");

	/* Map interrupt controller registers */
	intc_base = ioremap(M65832_INTC_BASE, M65832_PERIPH_SIZE);
	if (!intc_base) {
		pr_err("M65832: Failed to map interrupt controller\n");
		return;
	}

	/* Disable all interrupts initially */
	writel(0, intc_base + INTC_ENABLE);
	/* Drop any stale latched pending state before handlers are installed. */
	writel(0xffffffff, intc_base + INTC_CLEAR);
	{
		u32 en = readl(intc_base + INTC_ENABLE);
		u32 pend = readl(intc_base + INTC_PENDING);
		u32 st = readl(intc_base + INTC_STATUS);

		pr_info("M65832: INTC post-reset enable=%08x pending=%08x status=%08x\n",
			en, pend, st);
		if (en == 0xffffffff && pend == 0xffffffff && st == 0xffffffff) {
			pr_warn("M65832: INTC appears stuck-high; peripheral IRQ dispatch disabled\n");
			m65832_intc_disabled = true;
		}
	}

	/* Create IRQ domain */
	m65832_irq_domain = irq_domain_add_linear(NULL, M65832_NR_IRQS,
						  &m65832_irq_domain_ops, NULL);
	if (!m65832_irq_domain) {
		pr_err("M65832: Failed to create IRQ domain\n");
		return;
	}

	/*
	 * Pre-map all hardware IRQs so that request_irq(N, ...) works with
	 * a 1:1 hwirq-to-virq mapping.  irq_create_mapping calls our .map
	 * callback which sets the chip and handler for each virq.
	 */
	for (int i = 0; i < M65832_NR_IRQS; i++)
		m65832_virq_map[i] = irq_create_mapping(m65832_irq_domain, i);

	pr_info("M65832: IRQ controller initialized\n");
}

/*
 * Handle an IRQ - called from entry.S
 */
asmlinkage M65832_IRQ_OPTNONE void do_IRQ(struct pt_regs *regs)
{
	unsigned int pending;
	unsigned int irq;
	struct pt_regs *old_regs;

	old_regs = set_irq_regs(regs);
	irq_enter();

	/*
	 * The CPU-internal system timer (at M65832_SYSTIMER_CTRL) fires its
	 * interrupt directly into the CPU's IRQ line, bypassing the peripheral
	 * INTC.  Check its pending bit first, then fall through to INTC.
	 */
	{
		void __iomem *timer = (void __iomem *)M65832_SYSTIMER_CTRL;
		u32 ctrl = readl(timer + TIMER_CTRL);

		if (ctrl & TIMER_CTRL_IP) {
			/*
			 * Clear IP (W1C via TIMER_CTRL_IF) while preserving
			 * EN/PERIODIC/IE.  Writing TIMER_CTRL_IF alone would
			 * zero the whole register; include the existing
			 * non-pending bits so the timer keeps running.
			 */
			writel((ctrl & ~TIMER_CTRL_IP) | TIMER_CTRL_IF,
			       timer + TIMER_CTRL);
			irq = m65832_virq_map[M65832_TIMER_HW_IRQ];
			if (irq)
				generic_handle_irq(irq);
		}
	}

	if (!intc_base || m65832_intc_disabled)
		goto out;

	/* Read pending interrupts from the peripheral INTC */
	pending = readl(intc_base + INTC_PENDING);
	/*
	 * The INTC can report raw pending state for all lines; only dispatch
	 * lines that are currently enabled in the controller.
	 */
	pending &= readl(intc_base + INTC_ENABLE);

	for (irq = 0; irq < M65832_NR_IRQS; irq++) {
		if (!(pending & m65832_irq_bit[irq]))
			continue;

		/* Map to Linux IRQ and handle */
		{
			unsigned int virq = m65832_virq_map[irq];
			if (virq)
				generic_handle_irq(virq);
		}
	}

out:
	irq_exit();
	set_irq_regs(old_regs);
}

/*
 * Arch-specific IRQ setup
 */
int arch_setup_hwirq(unsigned int irq, int node)
{
	return 0;
}

void arch_teardown_hwirq(unsigned int irq)
{
}
