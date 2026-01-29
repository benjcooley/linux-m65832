// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Timer and clock handling for the M65832 architecture.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/sched_clock.h>

#include <asm/io.h>
#include <asm/setup.h>
#include <asm/irq.h>

/*
 * M65832 Timer registers
 * System timer at 0xFFFFF040-0xFFFFF048
 */
#define TIMER_BASE		0xFFFFF040

#define TIMER_CTRL		0x00	/* Control register */
#define TIMER_CMP		0x04	/* Compare value */
#define TIMER_COUNT		0x08	/* Current count (read-only) */

/* Timer control bits */
#define TIMER_CTRL_ENABLE	(1 << 0)	/* Enable timer */
#define TIMER_CTRL_IRQ_EN	(1 << 1)	/* Enable interrupt */
#define TIMER_CTRL_PERIODIC	(1 << 2)	/* Periodic mode */
#define TIMER_CTRL_IRQ_PENDING	(1 << 3)	/* IRQ pending (write 1 to clear) */

static void __iomem *timer_base;
static unsigned long timer_frequency;

/*
 * Read the timer count
 */
static u64 m65832_timer_read(struct clocksource *cs)
{
	if (!timer_base)
		return 0;
	return readl(timer_base + TIMER_COUNT);
}

static struct clocksource m65832_clocksource = {
	.name	= "m65832-timer",
	.rating	= 200,
	.read	= m65832_timer_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

/*
 * Clock event device callbacks
 */
static int m65832_timer_set_next_event(unsigned long delta,
				       struct clock_event_device *evt)
{
	unsigned long count;

	if (!timer_base)
		return -ENODEV;

	/* Read current count and set compare */
	count = readl(timer_base + TIMER_COUNT);
	writel(count + delta, timer_base + TIMER_CMP);

	/* Enable one-shot mode */
	writel(TIMER_CTRL_ENABLE | TIMER_CTRL_IRQ_EN, timer_base + TIMER_CTRL);

	return 0;
}

static int m65832_timer_set_periodic(struct clock_event_device *evt)
{
	unsigned long period;

	if (!timer_base)
		return -ENODEV;

	/* Calculate period for HZ ticks per second */
	period = timer_frequency / HZ;

	/* Set compare value */
	writel(period, timer_base + TIMER_CMP);

	/* Enable periodic mode */
	writel(TIMER_CTRL_ENABLE | TIMER_CTRL_IRQ_EN | TIMER_CTRL_PERIODIC,
	       timer_base + TIMER_CTRL);

	return 0;
}

static int m65832_timer_set_oneshot(struct clock_event_device *evt)
{
	if (!timer_base)
		return -ENODEV;

	/* Disable periodic mode */
	writel(TIMER_CTRL_ENABLE | TIMER_CTRL_IRQ_EN, timer_base + TIMER_CTRL);

	return 0;
}

static int m65832_timer_shutdown(struct clock_event_device *evt)
{
	if (timer_base)
		writel(0, timer_base + TIMER_CTRL);
	return 0;
}

static struct clock_event_device m65832_clockevent = {
	.name			= "m65832-timer",
	.features		= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating			= 200,
	.set_next_event		= m65832_timer_set_next_event,
	.set_state_periodic	= m65832_timer_set_periodic,
	.set_state_oneshot	= m65832_timer_set_oneshot,
	.set_state_shutdown	= m65832_timer_shutdown,
	.tick_resume		= m65832_timer_set_periodic,
};

/*
 * Timer interrupt handler
 */
static irqreturn_t m65832_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &m65832_clockevent;

	/* Clear interrupt */
	if (timer_base)
		writel(TIMER_CTRL_IRQ_PENDING, timer_base + TIMER_CTRL);

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

/*
 * Scheduler clock - use timer count directly
 */
static u64 notrace m65832_sched_clock_read(void)
{
	if (!timer_base)
		return 0;
	return readl(timer_base + TIMER_COUNT);
}

/*
 * Initialize timer subsystem
 */
void __init time_init(void)
{
	int ret;

	early_printk("M65832: Initializing timer\n");

	/* Get timer frequency from config or detect */
	timer_frequency = CONFIG_M65832_TIMER_FREQ;

	/* Map timer registers */
	timer_base = ioremap(TIMER_BASE, 0x10);
	if (!timer_base) {
		pr_err("M65832: Failed to map timer\n");
		return;
	}

	/* Register clocksource */
	ret = clocksource_register_hz(&m65832_clocksource, timer_frequency);
	if (ret) {
		pr_err("M65832: Failed to register clocksource\n");
		return;
	}

	/* Set up scheduler clock */
	sched_clock_register(m65832_sched_clock_read, 32, timer_frequency);

	/* Register clock event device */
	m65832_clockevent.cpumask = cpumask_of(0);
	clockevents_config_and_register(&m65832_clockevent, timer_frequency,
					0x10, 0xFFFFFFFF);

	/* Request timer IRQ (IRQ 5 or 6 typically) */
	ret = request_irq(5, m65832_timer_interrupt, IRQF_TIMER,
			  "m65832-timer", NULL);
	if (ret) {
		pr_err("M65832: Failed to request timer IRQ\n");
		return;
	}

	early_printk("M65832: Timer initialized at %lu Hz\n", timer_frequency);
}
