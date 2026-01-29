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

/*
 * M65832 Interrupt Controller registers
 * Base address: 0x10005000 (from memory map)
 */
#define M65832_INTC_BASE	0x10005000

#define INTC_PENDING		0x00	/* Pending interrupts (read) */
#define INTC_ENABLE		0x04	/* Enable mask */
#define INTC_DISABLE		0x08	/* Disable mask (write) */
#define INTC_ACK		0x0C	/* Acknowledge (write IRQ number) */
#define INTC_PRIORITY		0x10	/* Priority register */

static void __iomem *intc_base;
static struct irq_domain *m65832_irq_domain;

/*
 * Number of IRQ sources
 */
#define M65832_NR_IRQS		32

/*
 * IRQ chip callbacks
 */
static void m65832_irq_mask(struct irq_data *d)
{
	unsigned int irq = d->hwirq;

	if (intc_base)
		writel(1 << irq, intc_base + INTC_DISABLE);
}

static void m65832_irq_unmask(struct irq_data *d)
{
	unsigned int irq = d->hwirq;

	if (intc_base)
		writel(1 << irq, intc_base + INTC_ENABLE);
}

static void m65832_irq_ack(struct irq_data *d)
{
	unsigned int irq = d->hwirq;

	if (intc_base)
		writel(irq, intc_base + INTC_ACK);
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
	early_printk("M65832: Initializing interrupt controller\n");

	/* Map interrupt controller registers */
	intc_base = ioremap(M65832_INTC_BASE, 0x100);
	if (!intc_base) {
		pr_err("M65832: Failed to map interrupt controller\n");
		return;
	}

	/* Disable all interrupts initially */
	writel(0xFFFFFFFF, intc_base + INTC_DISABLE);

	/* Create IRQ domain */
	m65832_irq_domain = irq_domain_add_linear(NULL, M65832_NR_IRQS,
						  &m65832_irq_domain_ops, NULL);
	if (!m65832_irq_domain) {
		pr_err("M65832: Failed to create IRQ domain\n");
		return;
	}

	/* Set as default domain */
	irq_set_default_host(m65832_irq_domain);

	early_printk("M65832: IRQ controller initialized\n");
}

/*
 * Handle an IRQ - called from entry.S
 */
asmlinkage void do_IRQ(struct pt_regs *regs)
{
	unsigned int pending;
	unsigned int irq;
	struct pt_regs *old_regs;

	old_regs = set_irq_regs(regs);
	irq_enter();

	if (!intc_base) {
		pr_err("M65832: IRQ without controller!\n");
		goto out;
	}

	/* Read pending interrupts */
	pending = readl(intc_base + INTC_PENDING);

	while (pending) {
		/* Find first set bit */
		irq = __ffs(pending);
		pending &= ~(1 << irq);

		/* Map to Linux IRQ and handle */
		irq = irq_find_mapping(m65832_irq_domain, irq);
		if (irq)
			generic_handle_irq(irq);
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
