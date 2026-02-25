// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 Linux
 *
 * Architecture setup and initialization.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/memblock.h>
#include <linux/console.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include <asm/setup.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/platform.h>
#include <asm/sections.h>

/* Boot information from bootloader */
struct m65832_boot_info boot_info;

/* Kernel command line */
char cmd_line[COMMAND_LINE_SIZE] __initdata;

/* CPU clock frequency (Hz) */
unsigned long cpu_clock_freq = 100000000;	/* Default 100 MHz */
unsigned long timer_freq = 100000000;

/*
 * Ultra-early UART console for printk.
 * Uses the UART at 0x10006000 which is identity-mapped by head.S.
 * No ioremap needed -- we write to the physical address directly
 * through the peripheral identity mapping set up in the initial page tables.
 */
#define EARLY_UART_TX	(*(volatile unsigned int *)0x10006000)
#define EARLY_UART_ST	(*(volatile unsigned int *)0x10006004)
#define EARLY_UART_TXRDY 0x02

static void raw_uart_putc(unsigned char c)
{
	while (!(EARLY_UART_ST & EARLY_UART_TXRDY))
		;
	EARLY_UART_TX = c;
}

static void raw_uart_write(struct console *con, const char *s, unsigned n)
{
	while (n--) {
		if (*s == '\n')
			raw_uart_putc('\r');
		raw_uart_putc(*s);
		s++;
	}
}

static struct console raw_uart_console = {
	.name	= "rawuart",
	.write	= raw_uart_write,
	.flags	= CON_PRINTBUFFER | CON_ENABLED,
	.index	= -1,
};

/*
 * Register the raw UART console so printk output goes to the UART
 * immediately. Called at the very start of setup_arch.
 */
static void __init register_raw_console(void)
{
	register_console(&raw_uart_console);
}

/*
 * Early console output (before proper console is set up)
 * Uses the M65832 UART at M65832_UART_BASE (0x10006000)
 */
#ifdef CONFIG_M65832_EARLY_PRINTK
static void __iomem *early_uart;

static void early_putchar(char c)
{
	if (!early_uart)
		return;

	/* Wait for TX ready (UART_STATUS_TXRDY bit) */
	while (!(readl(early_uart + UART_STATUS) & UART_STATUS_TXRDY))
		;

	/* Write character to data register */
	writel(c, early_uart + UART_DATA);
}

void __init setup_early_printk(void)
{
	/*
	 * Map UART registers. Early in boot we may not have full
	 * ioremap, so we use the physical address directly if 
	 * MMU is not yet enabled.
	 */
#ifdef CONFIG_M65832_EARLYCON_UART_ADDRESS
	early_uart = ioremap(CONFIG_M65832_EARLYCON_UART_ADDRESS, M65832_PERIPH_SIZE);
#else
	early_uart = ioremap(M65832_UART_BASE, M65832_PERIPH_SIZE);
#endif
}

static void m65832_early_printk(const char *fmt, ...)
{
	va_list ap;
	char buf[256];
	char *p;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	for (p = buf; *p; p++) {
		if (*p == '\n')
			early_putchar('\r');
		early_putchar(*p);
	}
}
#endif

/*
 * Parse memory information from boot info or device tree
 */
static void __init setup_memory(void)
{
	unsigned long mem_start, mem_size;

	if (boot_info.mem_size) {
		mem_start = boot_info.mem_start;
		mem_size = boot_info.mem_size;
	} else {
		/* Default: 64MB starting at PHYS_OFFSET.
		 * Must not exceed the initial page table coverage set up
		 * by head.S (16 PTE tables x 4MB = 64MB). */
		mem_start = PHYS_OFFSET;
		mem_size = 64 * 1024 * 1024;
	}

	/*
	 * Ensure memory starts at or above PHYS_OFFSET.
	 * The kernel linear map maps physical PHYS_OFFSET to virtual
	 * PAGE_OFFSET, so physical addresses below PHYS_OFFSET would
	 * map below PAGE_OFFSET (user space) and be inaccessible.
	 */
	if (mem_start < PHYS_OFFSET) {
		mem_size -= (PHYS_OFFSET - mem_start);
		mem_start = PHYS_OFFSET;
	}

	m65832_early_printk("M65832: Memory: %luMB @ 0x%08lx\n",
		     mem_size / (1024 * 1024), mem_start);

	/* Register memory with memblock */
	memblock_add(mem_start, mem_size);

	/* Reserve kernel memory */
	memblock_reserve(__pa(_stext), _end - _stext);

	/* Reserve initial page tables */
	/* TODO: Reserve init_pg_dir */
}

/*
 * Main architecture setup function
 * Called from start_kernel() in init/main.c
 */
void __init setup_arch(char **cmdline_p)
{
	/* Register raw UART console so all printk output is visible */
	register_raw_console();

	/* Set up command line */
	if (boot_info.cmdline[0]) {
		strscpy(boot_command_line, boot_info.cmdline, COMMAND_LINE_SIZE);
	} else {
#ifdef CONFIG_CMDLINE
		strscpy(boot_command_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#else
		boot_command_line[0] = '\0';
#endif
	}
	strscpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = cmd_line;

	/*
	 * HACK(m65832-boot): Set preset_lpj so calibrate_delay() skips
	 * the convergence loop.  The timer interrupt works, but the
	 * delay-loop calibration takes too long on the emulator.
	 * TODO: Remove once lpj= cmdline parsing works or native
	 * calibrate_delay_direct() is implemented.
	 */
	{
		extern unsigned long preset_lpj;
		if (!preset_lpj)
			preset_lpj = CONFIG_M65832_TIMER_FREQ / HZ / 10;
	}

	m65832_early_printk("Command line: %s\n", cmd_line);

	/* Initialize memory management */
	setup_memory();

	/* Parse device tree if present */
#ifdef CONFIG_OF
	if (initial_boot_params) {
		unflatten_device_tree();
	}
#endif

	/* Initialize paging and memory zones */
	paging_init();

	/* Print CPU info */
	m65832_early_printk("M65832 CPU @ %lu MHz\n", cpu_clock_freq / 1000000);
}

/*
 * CPU initialization - called for each CPU
 */
void __init cpu_init(void)
{
	/* Set up exception vectors */
	/* TODO: trap_init() is called separately */

	/* Configure FPU if present */
#ifdef CONFIG_M65832_FPU
	/* TODO: FPU setup */
#endif
}

/* calibrate_delay is provided by init/calibrate.c */

/*
 * /proc/cpuinfo support
 */
static int show_cpuinfo(struct seq_file *m, void *v)
{
	seq_printf(m, "processor\t: 0\n");
	seq_printf(m, "cpu\t\t: M65832\n");
	seq_printf(m, "revision\t: 1\n");
	seq_printf(m, "cpu MHz\t\t: %lu.%02lu\n",
		   cpu_clock_freq / 1000000,
		   (cpu_clock_freq % 1000000) / 10000);
	seq_printf(m, "BogoMIPS\t: %lu.%02lu\n",
		   loops_per_jiffy / (500000 / HZ),
		   (loops_per_jiffy / (5000 / HZ)) % 100);
	seq_printf(m, "features\t: mmu");
#ifdef CONFIG_M65832_FPU
	seq_printf(m, " fpu");
#endif
	seq_printf(m, "\n");
	seq_printf(m, "hardware\t: %s\n",
#ifdef CONFIG_M65832_PLATFORM
		   CONFIG_M65832_PLATFORM
#else
		   "unknown"
#endif
		   );

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void c_stop(struct seq_file *m, void *v)
{
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= show_cpuinfo,
};
